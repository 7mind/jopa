#include "depend.h"
#include "control.h"
#include "ast.h"
#include "semantic.h"
#include "option.h"
#include "stream.h"


namespace Jikes { // Open namespace Jikes block
//
// Note that the types are ordered based on on the subtype relationship. We
// reverse the order here because the desired order for processing is the
// supertype relationship.
//
inline void TypeCycleChecker::ReverseTypeList()
{
    for (int head = 0, tail = type_list.Length() - 1;
         head < tail; head++, tail--)
    {
        TypeSymbol* temp = type_list[head];
        type_list[head] = type_list[tail];
        type_list[tail] = temp;
    }
}


void TypeCycleChecker::PartialOrder(Tuple<Semantic*>& semantic, int start)
{
    type_list.Reset();

    //
    // assert that the "index" of all types that should be checked is initially
    // set to OMEGA
    //
    for (unsigned i = start; i < semantic.Length(); i++)
    {
        Semantic* sem = semantic[i];
        for (unsigned k = 0;
             k < sem -> compilation_unit -> NumTypeDeclarations(); k++)
        {
            AstDeclaredType* declared =
                sem -> compilation_unit -> TypeDeclaration(k);
            if (declared -> EmptyDeclarationCast())
                continue;
            SemanticEnvironment* env =
                declared -> class_body -> semantic_environment;
            if (env) // type was successfully compiled thus far?
            {
                TypeSymbol* type = env -> Type();
                if (type -> index == OMEGA)
                   ProcessSubtypes(type);
            }
        }
    }

    ReverseTypeList();
}


void TypeCycleChecker::PartialOrder(SymbolSet& types)
{
    //
    // assert that the "index" of all types that should be checked is initially
    // set to OMEGA
    //
    for (TypeSymbol* type = (TypeSymbol*) types.FirstElement();
         type; type = (TypeSymbol*) types.NextElement())
    {
        if (type -> index == OMEGA)
            ProcessSubtypes(type);
    }

    ReverseTypeList();
}


void TypeCycleChecker::ProcessSubtypes(TypeSymbol* type)
{
    stack.Push(type);
    int indx = stack.Size();
    type -> index = indx;

    type -> subtypes_closure = new SymbolSet;
    type -> subtypes_closure -> Union(*(type -> subtypes));
    TypeSymbol* subtype;
    for (subtype = (TypeSymbol*) type -> subtypes -> FirstElement();
         subtype;
         subtype = (TypeSymbol*) type -> subtypes -> NextElement())
    {
        //
        // Only worry about top-level types.
        //
        if (subtype -> outermost_type != subtype)
            continue;
        if (subtype -> index == OMEGA)
             ProcessSubtypes(subtype);
        type -> index = Min(type -> index, subtype -> index);
        type -> subtypes_closure -> Union(*(subtype -> subtypes_closure));
    }

    if (type -> index == indx)
    {
        TypeSymbol* scc_subtype;
        do
        {
            scc_subtype = stack.Top();
            scc_subtype -> index = CYCLE_INFINITY;
            *(scc_subtype -> subtypes_closure) = *(type -> subtypes_closure);
            type_list.Next() = scc_subtype;
            stack.Pop();
        } while (scc_subtype != type);
    }
}


ConstructorCycleChecker::ConstructorCycleChecker(AstClassBody* class_body)
{
    for (unsigned k = 0; k < class_body -> NumConstructors(); k++)
    {
        AstConstructorDeclaration* constructor_declaration =
            class_body -> Constructor(k);
        if (constructor_declaration -> index == OMEGA)
            CheckConstructorCycles(constructor_declaration);
    }
}


void ConstructorCycleChecker::CheckConstructorCycles(AstConstructorDeclaration* constructor_declaration)
{
    stack.Push(constructor_declaration);
    int indx = stack.Size();
    constructor_declaration -> index = indx;

    AstConstructorDeclaration* called_constructor_declaration = NULL;

    AstMethodBody* constructor_block =
        constructor_declaration -> constructor_body;
    if (constructor_block -> explicit_constructor_opt)
    {
        AstThisCall* this_call =
            constructor_block -> explicit_constructor_opt -> ThisCallCast();
        MethodSymbol* called_constructor =
            (MethodSymbol*) (this_call ? this_call -> symbol : NULL);

        if (called_constructor)
        {
            called_constructor_declaration =
                (AstConstructorDeclaration*) called_constructor -> declaration;

            if (called_constructor_declaration -> index == OMEGA)
                CheckConstructorCycles(called_constructor_declaration);
            constructor_declaration -> index =
                Min(constructor_declaration -> index,
                    called_constructor_declaration -> index);
        }
    }

    if (constructor_declaration -> index == indx)
    {
        //
        // If the constructor_declaration is alone in its strongly connected
        // component (SCC), and it does not form a trivial cycle with itself,
        // pop it, mark it and return.
        //
        if (constructor_declaration == stack.Top() &&
            constructor_declaration != called_constructor_declaration)
        {
            stack.Pop();
            constructor_declaration -> index = CYCLE_INFINITY;
        }
        //
        // Otherwise, all elements in the stack up to (and including)
        // constructor_declaration form an SCC. Pop them off the stack, in
        // turn, mark them and issue the appropriate error message.
        //
        else
        {
            do
            {
                called_constructor_declaration = stack.Top();
                stack.Pop();
                called_constructor_declaration -> index = CYCLE_INFINITY;

                constructor_block =
                    (AstMethodBody*) called_constructor_declaration ->
                    constructor_body;
                AstMethodDeclarator* constructor_declarator =
                    called_constructor_declaration -> constructor_declarator;

                Semantic* sem = called_constructor_declaration ->
                    constructor_symbol -> containing_type ->
                    semantic_environment -> sem;
                sem -> ReportSemError(SemanticError::CIRCULAR_THIS_CALL,
                                      constructor_block -> explicit_constructor_opt,
                                      sem -> lex_stream -> NameString(constructor_declarator -> identifier_token));
            } while (called_constructor_declaration != constructor_declaration);
        }
    }
}


//
// assert that the "index" of all types that should be checked is initially
// set to OMEGA
//
void TypeDependenceChecker::PartialOrder()
{
    for (FileSymbol* file_symbol = (FileSymbol*) file_set.FirstElement();
         file_symbol;
         file_symbol = (FileSymbol*) file_set.NextElement())
    {
        for (unsigned j = 0; j < file_symbol -> types.Length(); j++)
        {
            TypeSymbol* type = file_symbol -> types[j];
            if (type -> incremental_index == OMEGA)
                ProcessType(type);
        }
    }

    for (unsigned k = 0; k < type_trash_bin.Length(); k++)
    {
        TypeSymbol* type = type_trash_bin[k];
        if (type -> incremental_index == OMEGA)
            ProcessType(type);
    }
}


void TypeDependenceChecker::ProcessType(TypeSymbol* type)
{
    stack.Push(type);
    int indx = stack.Size();
    type -> incremental_index = indx;

    // if dependents is reflexive make it non-reflexive - saves time !!!
    type -> dependents -> RemoveElement(type);
    type -> dependents_closure = new SymbolSet;
    // compute reflexive transitive closure
    type -> dependents_closure -> AddElement(type);
    TypeSymbol* dependent;
    for (dependent = (TypeSymbol*) type -> dependents -> FirstElement();
         dependent;
         dependent = (TypeSymbol*) type -> dependents -> NextElement())
    {
        if (dependent -> incremental_index == OMEGA)
             ProcessType(dependent);
        type -> incremental_index = Min(type -> incremental_index,
                                        dependent -> incremental_index);
        type -> dependents_closure ->
            Union(*(dependent -> dependents_closure));
    }

    if (type -> incremental_index == indx)
    {
        TypeSymbol* scc_dependent;
        do
        {
            scc_dependent = stack.Top();
            scc_dependent -> incremental_index = CYCLE_INFINITY;
            *(scc_dependent -> dependents_closure) =
                *(type -> dependents_closure);
            type_list.Next() = scc_dependent;
            stack.Pop();
        } while (scc_dependent != type);
    }
}


void TypeDependenceChecker::OutputMake(FILE* outfile, char* output_name,
                                       Tuple<FileSymbol*>& file_list)
{
    assert(outfile);

    for (unsigned i = 0; i < file_list.Length(); i++)
    {
        FileSymbol* file_symbol = file_list[i];
        char* name = file_symbol -> FileName();
        int length = file_symbol -> FileNameLength() -
            (file_symbol -> IsJava() ? FileSymbol::java_suffix_length
             : FileSymbol::class_suffix_length);

        char* class_name = new char[length + FileSymbol::class_suffix_length +
                                   1];
        char* java_name =
            new char[length + FileSymbol::java_suffix_length + 1];

        strncpy(class_name, name, length);
        strcpy(&class_name[length], FileSymbol::class_suffix);
        strncpy(java_name, name, length);
        strcpy(&java_name[length], FileSymbol::java_suffix);

        fprintf(outfile, "%s : %s\n", output_name, java_name);

        if (i > 0) // Not the first file in the list
        {
            fprintf(outfile, "%s : %s\n", output_name, class_name);
        }

        delete [] class_name;
        delete [] java_name;
    }
}


void TypeDependenceChecker::OutputMake(FileSymbol* file_symbol)
{
    //
    //
    //
    const char* name;
    char* buf = NULL;
    int length;

    if (control -> option.directory == NULL)
    {
        name = file_symbol -> FileName();
        length = file_symbol -> FileNameLength() -
            (file_symbol -> IsJava() ? FileSymbol::java_suffix_length
             : FileSymbol::class_suffix_length);
    }
    else
    {
        name = file_symbol -> Utf8Name();
        length = strlen(name);

        DirectorySymbol* dir_symbol = file_symbol -> OutputDirectory();
        char* dir_name = dir_symbol -> DirectoryName();
        int dir_length = strlen(dir_name);

        buf = new char[length + FileSymbol::class_suffix_length + dir_length +
                      2];
        strcpy(buf, dir_name);

#ifdef UNIX_FILE_SYSTEM
        buf[dir_length] = (char)U_SLASH;
#elif defined(WIN32_FILE_SYSTEM)
        buf[dir_length] = (char)U_BACKSLASH;
#endif

        strcpy(&buf[dir_length+1], name);
        name = buf;
        length = dir_length + 1 + length;
    }



    char* output_name = new char[length + FileSymbol::class_suffix_length + 1];
    char* u_name = new char[length + strlen(StringConstant::U8S_DO_u) + 1];

    strncpy(output_name, name, length);
    strncpy(u_name, name, length);
    strcpy(&output_name[length], FileSymbol::class_suffix);
    strcpy(&u_name[length], StringConstant::U8S_DO_u);

    //
    //
    //
    SymbolSet file_set;
    for (unsigned i = 0; i < file_symbol -> types.Length(); i++)
    {
        TypeSymbol* type = file_symbol -> types[i];
        TypeSymbol* parent;
        for (parent = (TypeSymbol*) type -> parents_closure -> FirstElement();
             parent;
             parent = (TypeSymbol*) type -> parents_closure -> NextElement())
        {
            FileSymbol* symbol = parent -> file_symbol;
            if (symbol && (! symbol -> IsZip()))
                file_set.AddElement(symbol);
        }
    }
    file_set.RemoveElement(file_symbol);

    //
    //
    //
    Tuple<FileSymbol*> file_list(file_set.Size());
    file_list.Next() = file_symbol;
    for (FileSymbol* symbol = (FileSymbol*) file_set.FirstElement();
         symbol; symbol = (FileSymbol*) file_set.NextElement())
        file_list.Next() = symbol;

    FILE* outfile = SystemFopen(u_name, "w");
    if (outfile == NULL)
        Coutput << "*** Cannot open output Makefile " << u_name << endl;
    else
    {
        OutputMake(outfile, output_name, file_list);
        fclose(outfile);
    }

    delete [] output_name;
    delete [] u_name;
    if (control -> option.directory)
        delete [] buf;
}


void TypeDependenceChecker::OutputDependences()
{
    SymbolSet new_file_set;
    unsigned i;
    for (i = 0; i < type_list.Length(); i++)
    {
        TypeSymbol* type = type_list[i];
        type -> parents_closure = new SymbolSet;

        FileSymbol* file_symbol = type -> file_symbol;
        if (file_symbol && (! file_symbol -> IsZip()))
            new_file_set.AddElement(file_symbol);
    }

    for (i = 0; i < type_list.Length(); i++)
    {
        TypeSymbol* parent = type_list[i];
        TypeSymbol* dependent;
        for (dependent = (TypeSymbol*) parent -> dependents_closure -> FirstElement();
             dependent;
             dependent = (TypeSymbol*) parent -> dependents_closure -> NextElement())
        {
            dependent -> parents_closure -> AddElement(parent);
        }
    }

    for (FileSymbol* symbol = (FileSymbol*) new_file_set.FirstElement();
         symbol; symbol = (FileSymbol*) new_file_set.NextElement())
    {
        OutputMake(symbol);
    }

    for (i = 0; i < type_list.Length(); i++)
    {
        TypeSymbol* type = type_list[i];
        delete type -> parents_closure;
        type -> parents_closure = NULL;
    }
}


void TopologicalSort::Process(TypeSymbol* type)
{
    pending -> AddElement(type);

    TypeSymbol* super_type;
    for (super_type = (TypeSymbol*) type -> supertypes_closure -> FirstElement();
         super_type;
         super_type = (TypeSymbol*) type -> supertypes_closure -> NextElement())
    {
        if (type_collection.IsElement(super_type))
        {
            if (! pending -> IsElement(super_type))
                Process(super_type);
        }
    }

    type_list.Next() = type;
}


void TopologicalSort::Sort()
{
    type_list.Reset();

    for (TypeSymbol* type = (TypeSymbol*) type_collection.FirstElement();
         type; type = (TypeSymbol*) type_collection.NextElement())
    {
        if (! pending -> IsElement(type))
            Process(type);
    }

    pending -> SetEmpty();
}


TopologicalSort::TopologicalSort(SymbolSet& type_collection_,
                                 Tuple<TypeSymbol*>& type_list_)
    : type_collection(type_collection_)
    , type_list(type_list_)
{
    pending = new SymbolSet(type_collection.Size());
}


TopologicalSort::~TopologicalSort()
{
    delete pending;
}


//
// Depend on the base enclosing class. For example, with class A { class B{} },
// using the type A.B[] will add a dependence on A, because it is A.java that
// must exist for the compiler to redefine B.class, and B.class that will be
// used by the VM to define B[]. We cannot add dependences on the primitive
// types, because there is no .java file that defines them.
// 
void Semantic::AddDependence(TypeSymbol* base_type, TypeSymbol* parent_type,
                             bool static_access)
{
    assert(! base_type -> IsArray() && ! base_type -> Primitive());
    if (parent_type -> IsArray())
    {
        parent_type = parent_type -> base_type;
    }
    if (base_type -> Bad() || parent_type -> Bad() ||
        parent_type == control.null_type || parent_type -> Primitive())
    {
        return;
    }
    base_type = base_type -> outermost_type;
    parent_type = parent_type -> outermost_type;
    parent_type -> dependents -> AddElement(base_type);
    if (static_access)
        base_type -> static_parents -> AddElement(parent_type);
    else base_type -> parents -> AddElement(parent_type);

    //
    // It is not possible to import from the unnamed package, and without
    // imports, it is impossible to reference a class in the unnamed
    // package from a package.
    //
    assert(parent_type -> ContainingPackage() != control.UnnamedPackage() ||
           base_type -> ContainingPackage() == control.UnnamedPackage());
}



} // Close namespace Jikes block
#endif
