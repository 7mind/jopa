#include "platform.h"
#include "control.h"
#include "scanner.h"
#include "semantic.h"
#include "case.h"
#include "set.h"
#include "parser_facade.h"


namespace Jopa { // Open namespace Jopa block
void Control::RemoveTrashedTypes(SymbolSet& type_trash_set)
{
    TypeSymbol* type;

    //
    // For each type T that is going to be trashed, and for each parent P of T
    // that is not itself being trashed, remove T from the set of dependents of
    // P. If T is a subtype of P it is also removed from the subtypes set.
    //
    for (type = (TypeSymbol*) type_trash_set.FirstElement();
         type; type = (TypeSymbol*) type_trash_set.NextElement())
    {
        TypeSymbol* parent;
        for (parent = (TypeSymbol*) type -> static_parents -> FirstElement();
             parent;
             parent = (TypeSymbol*) type -> static_parents -> NextElement())
        {
            if (! type_trash_set.IsElement(parent))
            {
                parent -> dependents -> RemoveElement(type);
                parent -> subtypes -> RemoveElement(type);
            }
        }

        for (parent = (TypeSymbol*) type -> parents -> FirstElement();
             parent;
             parent = (TypeSymbol*) type -> parents -> NextElement())
        {
            if (! type_trash_set.IsElement(parent))
            {
                parent -> dependents -> RemoveElement(type);
                parent -> subtypes -> RemoveElement(type);
            }
        }
    }

    //
    // We can now safely delete the type.
    //
    for (type = (TypeSymbol*) type_trash_set.FirstElement();
         type; type = (TypeSymbol*) type_trash_set.NextElement())
    {
        PackageSymbol* package = type -> ContainingPackage();

        //
        // If a type that is about to be trashed was read in via a class file,
        // remove the class file. Note that invoking RemoveElement for a file
        // that it does not contain has no ill effect.
        //
        FileSymbol* file_symbol = type -> file_symbol;
        if (file_symbol && type -> Identity() == file_symbol -> Identity())
            input_class_file_set.RemoveElement(file_symbol);

        //
        // If a type that is about to be trashed was contained in the
        // unnamed_package, remove it from the set "unnamed_package_types"
        //
        if (package == unnamed_package)
            unnamed_package_types.RemoveElement(type);

        //
        // Remove the type from its containing package.
        //
        package -> DeleteTypeSymbol(type);
    }
}


inline DirectoryEntry* Control::FindInputFile(FileSymbol* file_symbol)
{
    int length
        = file_symbol -> Utf8NameLength() + FileSymbol::java_suffix_length;

    char* java_name = new char[length + 1]; // +1 for '\0'
    strcpy(java_name, file_symbol -> Utf8Name());
    strcat(java_name, FileSymbol::java_suffix);

    DirectoryEntry* java_entry
        = file_symbol -> directory_symbol -> FindEntry(java_name, length);

    delete [] java_name;
    return java_entry;
}


//
// For each file whose associated source (".java") has changed, add it to the
// list to be recompiled...
//
void Control::FindMoreRecentInputFiles(SymbolSet& file_candidates)
{
    FileSymbol* file_symbol;
    for (file_symbol = (FileSymbol*) file_candidates.FirstElement();
         file_symbol;
         file_symbol = (FileSymbol*) file_candidates.NextElement())
    {
        //
        // If the type is not zipped and it is not already contained in the
        // recompilation set, then check it...
        //
        if ((! file_symbol -> IsZip()) &&
            (! recompilation_file_set.IsElement(file_symbol)) &&
            (! expired_file_set.IsElement(file_symbol)))
        {
            //
            // If there is no java source file or its time stamp is not newer
            // than file_symbol then reset file_symbol to NULL. Otherwise,
            // reset file symbol to the newer file.
            //
            DirectoryEntry* java_entry = FindInputFile(file_symbol);
            if (! java_entry)
            {
                // A source file that was compiled in the previous pass no
                // longer exists.
                if (file_symbol -> IsJava())
                    expired_file_set.AddElement(file_symbol);
            }
            else if (java_entry -> Mtime() > file_symbol -> mtime)
            {
                // A newer file was found.
                file_symbol -> mtime = java_entry -> Mtime();
                recompilation_file_set.AddElement(file_symbol);
            }
        }
    }
}


void Control::RereadDirectory(DirectorySymbol* directory_symbol)
{
    directory_symbol -> ResetDirectory();

    for (unsigned i = 0; i < directory_symbol -> subdirectories.Length(); i++)
        RereadDirectory(directory_symbol -> subdirectories[i]);
}


void Control::RereadDirectories()
{
    for (unsigned i = (dot_classpath_index == 0 ? 0 : 1);
         i < classpath.Length(); i++)
    {
        PathSymbol* path_symbol = classpath[i];
        if (! path_symbol -> IsZip())
            RereadDirectory(path_symbol -> RootDirectory());
    }
}


void Control::ComputeRecompilationSet(TypeDependenceChecker& dependence_checker)
{
    SymbolSet type_trash_set;

    //
    // Find out if any source files has been touched since the last
    // compilation and add all such files to recompilation_file_set.
    //
    FindMoreRecentInputFiles(dependence_checker.file_set);

    //
    // Before messing with the files, compute a list of all the types that
    // have just been compiled. We need to do this here as we will be
    // "Resetting" and "reScanning" some files in the loop below, which in
    // effect removes the set of types to which they were associated in the
    // previous compilation.
    //
    int length_estimate = input_java_file_set.Size(); // problem size estimate
    Tuple<TypeSymbol*> input_types(length_estimate * 2);
    FileSymbol* file_symbol;
    for (file_symbol = (FileSymbol*) input_java_file_set.FirstElement();
         file_symbol;
         file_symbol = (FileSymbol*) input_java_file_set.NextElement())
    {
        for (unsigned i = 0; i < file_symbol -> types.Length(); i++)
            input_types.Next() = file_symbol -> types[i];
    }

    //
    // Declare the closure set, and initialize it with the Union over the
    // closure of the types in the trash_bin. Essentially, we want to catch
    // all "compiled" types in the compilation that has a dependence on these
    // bad types.
    //
    SymbolSet dependents_closure(length_estimate);
    for (unsigned i = 0; i < type_trash_bin.Length(); i++)
    {
        TypeSymbol* type = type_trash_bin[i];
        if (! dependents_closure.IsElement(type))
        {
            if (type -> dependents_closure)
                 dependents_closure.Union(*type -> dependents_closure);
            else dependents_closure.AddElement(type);
        }
    }

    //
    // Compute the set of types from the recompilation set that needs to be
    // recompiled and update the recompilation file set.
    //
    SymbolSet new_set(length_estimate),
              file_seen(length_estimate);
    new_set = recompilation_file_set;
    new_set.Union(expired_file_set);
    file_seen = new_set;

    // How much space do we need for a package declaration? estimate 64 tokens.
    StoragePool* ast_pool = new StoragePool(64);

    //
    // As long as there is a new_set of files to process,...
    //
    do
    {
        //
        // For each file in new_set, compute the reflexive transitive closure
        // of all types contained in that file. Next, reset and rescan the
        // file. If the scan was successful, iterate over the new list of
        // types to see if any of them had already been introduced in the
        // previous compilation via a class file. If so, add all such types to
        // the dependents closure.
        //
        for (FileSymbol* file_symbol = (FileSymbol*) new_set.FirstElement();
             file_symbol;
             file_symbol = (FileSymbol*) new_set.NextElement())
        {
            for (unsigned i = 0; i < file_symbol -> types.Length(); i++)
            {
                TypeSymbol* type = file_symbol -> types[i];
                if (! dependents_closure.IsElement(type))
                {
                    if (type -> dependents_closure)
                        dependents_closure.Union(*type -> dependents_closure);
                    else dependents_closure.AddElement(type);
                }
            }

            if (! expired_file_set.IsElement(file_symbol))
            {
                file_symbol -> Reset();
                file_symbol -> SetJava();

                scanner -> Scan(file_symbol);

                LexStream* lex_stream = file_symbol -> lex_stream;
                if (lex_stream) // did we have a successful scan!
                {
                    AstPackageDeclaration* package_declaration
                        = parser_facade->packageHeaderParse(lex_stream, ast_pool);
                    PackageSymbol* package
                        = (package_declaration
                           ? FindOrInsertPackage(lex_stream,
                                                 package_declaration -> name)
                           : unnamed_package);
                    ast_pool -> Reset();

                    //
                    // If the file contained more than one type, only the main
                    // one would have been deleted. We now delete the others if
                    // any...
                    //
                    for (unsigned k = 0; k < lex_stream -> NumTypes(); k++)
                    {
                        TokenIndex identifier_token
                            = lex_stream -> Next(lex_stream -> Type(k));
                        NameSymbol* name_symbol =
                            lex_stream -> NameSymbol(identifier_token);
                        if (name_symbol)
                        {
                            TypeSymbol* type
                                = package -> FindTypeSymbol(name_symbol);
                            if (type && (! dependents_closure.IsElement(type)))
                            {
                                if (type -> dependents_closure)
                                    dependents_closure.Union(*type -> dependents_closure);
                                else dependents_closure.AddElement(type);
                            }
                        }
                    }
                }
            }
        }

        //
        // Iterate over the dependents_closure set. For each type T, add it to
        // the trash pile. If the file with which it is associated had not yet
        // been processed, mark it as having been "seen" and add it to the
        // new_set to be considered later. If the file had already been
        // processed but not yet added to the recompilation set, add it to the
        // recompilation set, read it in and if it contains types other than
        // the main one (that had previously been read in via class files) add
        // those new types to the trash pile.
        //
        new_set.SetEmpty();
        TypeSymbol* type;
        for (type = (TypeSymbol*) dependents_closure.FirstElement();
             type;
             type = (TypeSymbol*) dependents_closure.NextElement())
        {
            type_trash_set.AddElement(type);

            FileSymbol* file_symbol = type -> file_symbol;
            if (file_symbol && (! file_seen.IsElement(file_symbol)))
            {
                file_seen.AddElement(file_symbol);
                new_set.AddElement(file_symbol);
                file_symbol -> mtime = 0; // to force a reread of the file.
            }
        }

        //
        // Check that the files in new_set exist, and if so, add them to the
        // recompilation_file_set. Note that if they exist, they will be added
        // because before a file is added to new_set its time stamp is reset
        // to 0. See loop above...
        //
        FindMoreRecentInputFiles(new_set);

        //
        // Empty out the dependents_closure set for the next round.
        //
        dependents_closure.SetEmpty();
    } while (! new_set.IsEmpty());

    delete ast_pool;

    //
    // Clean up the types that were compiled in the previous compilation pass.
    //
    for (unsigned j = 0; j < input_types.Length(); j++)
        input_types[j] -> RemoveCompilationReferences();

    //
    // Reset the closure sets in all the types that were considered in the
    // dependence checker.
    //
    Tuple<TypeSymbol*>& type_list = dependence_checker.TypeList();
    for (unsigned k = 0; k < type_list.Length(); k++)
    {
        TypeSymbol* type = type_list[k];

        type -> index = TypeCycleChecker::OMEGA;
        type -> unit_index = TypeCycleChecker::OMEGA;
        type -> incremental_index = TypeCycleChecker::OMEGA;
        delete type -> dependents_closure;
        type -> dependents_closure = NULL;
    }

    //
    // Remove all dependence edges that are no longer valid.
    //
    RemoveTrashedTypes(type_trash_set);
}


//
// Check whether or not there are files to be recompiled.
//
bool Control::IncrementalRecompilation()
{
    //
    // Empty out the type lookup table so that it does not continue
    // to point to a type that is deleted here.
    //
    type_table.SetEmpty();

    SymbolSet candidates(input_java_file_set.Size() +
                         input_class_file_set.Size() +
                         recompilation_file_set.Size());

    if (! recompilation_file_set.IsEmpty())
        candidates = recompilation_file_set;
    else
    {
        Ostream out;
        out.StandardOutput();
        out << endl << "Incremental: Enter to continue or q + Enter to quit: "
            << flush;

        char ch;
        // See if the user types Q or presses enter/escape or sends an EOF
        while (1) {
            cin.get(ch);
            if (cin.eof() || (ch == U_q) || (ch == U_Q)) {
                return false;
            }
            if ((ch == U_ESCAPE) || (ch == U_LINE_FEED)) {
                break;
            }
        }

        candidates = input_java_file_set;
        candidates.Union(input_class_file_set);
    }

    if (!candidates.IsEmpty())
    {
        TypeDependenceChecker dependence_checker(this, candidates,
                                                 type_trash_bin);
        dependence_checker.PartialOrder();

        //
        // Compute the initial set of files that need to be recompiled. Place
        // them in recompilation_file_set.
        //
        RereadDirectories();

        ComputeRecompilationSet(dependence_checker);
    }

    //
    // Starting with the initial recompilation_file_set, complete the
    // computation of the set of files that need to be recompiled. (Add all
    // new files to recompilation_file_set). Also, complete the computation of
    // type_trash_set, the set of files that should be removed from the
    // database as they will be recompiled.
    //
    fprintf(stderr, "%s", (recompilation_file_set.IsEmpty() &&
                           expired_file_set.IsEmpty()
                           ? "\nnothing changed...\n" : "\nok...\n"));
    fflush(stderr);
    return true;
}


} // Close namespace Jopa block

