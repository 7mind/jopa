// ByteCode initialization and method setup
// Split from bytecode.cpp for maintainability

#include "bytecode.h"
#include "ast.h"
#include "class.h"
#include "control.h"
#include "semantic.h"
#include "stream.h"
#include "symbol.h"
#include "table.h"
#include "option.h"

namespace Jopa {

void ByteCode::GenerateCode()
{
    AstClassBody* class_body = unit_type -> declaration;
    unsigned i;

    //
    // Process static variables.
    //
    for (i = 0; i < class_body -> NumClassVariables(); i++)
    {
        AstFieldDeclaration* field_decl = class_body -> ClassVariable(i);
        for (unsigned vi = 0;
             vi < field_decl -> NumVariableDeclarators(); vi++)
        {
            AstVariableDeclarator* vd = field_decl -> VariableDeclarator(vi);
            DeclareField(vd -> symbol);
        }
    }

    //
    // Process enum constants as static final fields.
    //
    if (class_body -> owner && class_body -> owner -> EnumDeclarationCast())
    {
        AstEnumDeclaration* enum_decl = class_body -> owner -> EnumDeclarationCast();
        for (i = 0; i < enum_decl -> NumEnumConstants(); i++)
        {
            AstEnumConstant* enum_constant = enum_decl -> EnumConstant(i);
            if (enum_constant -> field_symbol)
                DeclareField(enum_constant -> field_symbol);
        }
    }

    //
    // Process instance variables.  We separate constant fields from others,
    // because in 1.4 or later, constant fields are initialized before the
    // call to super() in order to obey semantics of JLS 13.1.
    //
    Tuple<AstVariableDeclarator*> constant_instance_fields
        (unit_type -> NumVariableSymbols());
    for (i = 0; i < class_body -> NumInstanceVariables(); i++)
    {
        AstFieldDeclaration* field_decl  = class_body -> InstanceVariable(i);
        for (unsigned vi = 0;
             vi < field_decl -> NumVariableDeclarators(); vi++)
        {
            AstVariableDeclarator* vd = field_decl -> VariableDeclarator(vi);
            VariableSymbol* vsym = vd -> symbol;
            DeclareField(vsym);
            if (vd -> variable_initializer_opt && vsym -> initial_value)
            {
                AstExpression* init = vd -> variable_initializer_opt ->
                       ExpressionCast();
                assert(init);
                assert(init -> IsConstant() && vd -> symbol -> ACC_FINAL());
                (void)init;
                constant_instance_fields.Next() = vd;
            }
        }
    }

    //
    // Process synthetic fields (this$0, local shadow parameters, $class...,
    // $array..., $noassert).
    //
    if (unit_type -> EnclosingType())
        DeclareField(unit_type -> EnclosingInstance());
    for (i = 0; i < unit_type -> NumConstructorParameters(); i++)
        DeclareField(unit_type -> ConstructorParameter(i));
    for (i = 0; i < unit_type -> NumClassLiterals(); i++)
        DeclareField(unit_type -> ClassLiteral(i));
    VariableSymbol* assert_variable = unit_type -> AssertVariable();
    if (assert_variable)
    {
        assert(! control.option.noassert);
        DeclareField(assert_variable);
        if (control.option.target < JopaOption::SDK1_4)
        {
            semantic.ReportSemError(SemanticError::ASSERT_UNSUPPORTED_IN_TARGET,
                                    unit_type -> declaration,
                                    unit_type -> ContainingPackageName(),
                                    unit_type -> ExternalName());
            assert_variable = NULL;
        }
    }

    //
    // Process declared methods.
    //
    for (i = 0; i < class_body -> NumMethods(); i++)
    {
        AstMethodDeclaration* method = class_body -> Method(i);
        if (method -> method_symbol)
        {
            int method_index = methods.NextIndex(); // index for method
            BeginMethod(method_index, method -> method_symbol);
            if (method -> method_body_opt) // not an abstract method ?
                EmitBlockStatement(method -> method_body_opt);
            EndMethod(method_index, method -> method_symbol);
        }
    }

    //
    // Process synthetic methods (access$..., class$).
    //
    for (i = 0; i < unit_type -> NumPrivateAccessMethods(); i++)
    {
        int method_index = methods.NextIndex(); // index for method
        MethodSymbol* method_sym = unit_type -> PrivateAccessMethod(i);
        AstMethodDeclaration* method = method_sym -> declaration ->
            MethodDeclarationCast();
        assert(method);
        BeginMethod(method_index, method_sym);
        EmitBlockStatement(method -> method_body_opt);
        EndMethod(method_index, method_sym);
    }
    MethodSymbol* class_literal_sym = unit_type -> ClassLiteralMethod();
    if (class_literal_sym)
    {
        int method_index = methods.NextIndex(); // index for method
        BeginMethod(method_index, class_literal_sym);
        GenerateClassAccessMethod();
        EndMethod(method_index, class_literal_sym);
    }

    //
    // Generate bridge methods for generic covariant overrides
    //
    for (i = 0; i < unit_type -> NumMethodSymbols(); i++)
    {
        MethodSymbol* method = unit_type -> MethodSym(i);
        if (method -> NumGeneratedBridges() > 0)
        {
            for (unsigned j = 0; j < method -> NumGeneratedBridges(); j++)
            {
                MethodSymbol* bridge = method -> GeneratedBridge(j);
                int method_index = methods.NextIndex();
                BeginMethod(method_index, bridge);
                GenerateBridgeMethod(bridge);
                EndMethod(method_index, bridge);
            }
        }
    }

    //
    // Generate enum synthetic methods (values(), valueOf())
    //
    if (unit_type -> IsEnum())
    {
        // Find values() method
        NameSymbol* values_name = control.FindOrInsertName(L"values", 6);
        MethodSymbol* values_method = unit_type -> FindMethodSymbol(values_name);
        if (values_method)
        {
            int method_index = methods.NextIndex();
            BeginMethod(method_index, values_method);
            GenerateEnumValuesMethod();
            EndMethod(method_index, values_method);
        }

        // Find valueOf(String) method
        NameSymbol* valueOf_name = control.FindOrInsertName(L"valueOf", 7);
        for (i = 0; i < unit_type -> NumMethodSymbols(); i++)
        {
            MethodSymbol* method = unit_type -> MethodSym(i);
            if (method -> Identity() == valueOf_name &&
                method -> NumFormalParameters() == 1 &&
                method -> FormalParameter(0) -> Type() == control.String())
            {
                int method_index = methods.NextIndex();
                BeginMethod(method_index, method);
                GenerateEnumValueOfMethod();
                EndMethod(method_index, method);
                break;
            }
        }
    }

    //
    // Process the instance initializer.
    //
    bool has_instance_initializer = false;
    if (unit_type -> instance_initializer_method)
    {
        AstMethodDeclaration* declaration = (AstMethodDeclaration*)
            unit_type -> instance_initializer_method -> declaration;
        AstBlock* init_block = declaration -> method_body_opt;
        if (! IsNop(init_block))
        {
            int method_index = methods.NextIndex(); // index for method
            BeginMethod(method_index,
                        unit_type -> instance_initializer_method);
            bool abrupt = EmitBlockStatement(init_block);
            if (! abrupt)
                PutOp(OP_RETURN);
            EndMethod(method_index, unit_type -> instance_initializer_method);
            has_instance_initializer = true;
        }
    }

    //
    // Process all constructors (including synthetic ones).
    //
    if (class_body -> default_constructor)
        CompileConstructor(class_body -> default_constructor,
                           constant_instance_fields, has_instance_initializer);
    else
    {
        for (i = 0; i < class_body -> NumConstructors(); i++)
            CompileConstructor(class_body -> Constructor(i),
                               constant_instance_fields,
                               has_instance_initializer);
    }
    for (i = 0; i < unit_type -> NumPrivateAccessConstructors(); i++)
    {
        MethodSymbol* constructor_sym =
            unit_type -> PrivateAccessConstructor(i);
        AstConstructorDeclaration* constructor =
            constructor_sym -> declaration -> ConstructorDeclarationCast();
        CompileConstructor(constructor, constant_instance_fields,
                           has_instance_initializer);
    }

    //
    // Process the static initializer.
    //
    if (unit_type -> static_initializer_method)
    {
        AstMethodDeclaration* declaration = (AstMethodDeclaration*)
            unit_type -> static_initializer_method -> declaration;
        AstBlock* init_block = declaration -> method_body_opt;
        if (assert_variable || ! IsNop(init_block))
        {
            int method_index = methods.NextIndex(); // index for method
            BeginMethod(method_index, unit_type -> static_initializer_method);
            if (assert_variable)
                GenerateAssertVariableInitializer(unit_type -> outermost_type,
                                                  assert_variable);
            bool abrupt = EmitBlockStatement(init_block);
            if (! abrupt)
                PutOp(OP_RETURN);
            EndMethod(method_index, unit_type -> static_initializer_method);
        }
    }

    FinishCode();

    //
    // Check for overflow.
    //
    if (constant_pool.Length() > 65535)
    {
        semantic.ReportSemError(SemanticError::CONSTANT_POOL_OVERFLOW,
                                unit_type -> declaration,
                                unit_type -> ContainingPackageName(),
                                unit_type -> ExternalName());
    }
    if (interfaces.Length() > 65535)
    {
        // Interface overflow implies constant pool overflow.
        semantic.ReportSemError(SemanticError::INTERFACES_OVERFLOW,
                                unit_type -> declaration,
                                unit_type -> ContainingPackageName(),
                                unit_type -> ExternalName());
    }
    if (fields.Length() > 65535)
    {
        // Field overflow implies constant pool overflow.
        semantic.ReportSemError(SemanticError::FIELDS_OVERFLOW,
                                unit_type -> declaration,
                                unit_type -> ContainingPackageName(),
                                unit_type -> ExternalName());
    }
    if (methods.Length() > 65535)
    {
        // Method overflow implies constant pool overflow.
        semantic.ReportSemError(SemanticError::METHODS_OVERFLOW,
                                unit_type -> declaration,
                                unit_type -> ContainingPackageName(),
                                unit_type -> ExternalName());
    }
    if (string_overflow)
    {
        semantic.ReportSemError(SemanticError::STRING_OVERFLOW,
                                unit_type -> declaration,
                                unit_type -> ContainingPackageName(),
                                unit_type -> ExternalName());
    }
    if (library_method_not_found)
    {
        semantic.ReportSemError(SemanticError::LIBRARY_METHOD_NOT_FOUND,
                                unit_type -> declaration,
                                unit_type -> ContainingPackageName(),
                                unit_type -> ExternalName());
    }

    if (semantic.NumErrors() == 0)
        Write(unit_type);
#ifdef JOPA_DEBUG
    if (control.option.debug_dump_class)
        Print();
#endif // JOPA_DEBUG
}


//
// initialized_fields is a list of fields needing code to initialize.
//
void ByteCode::CompileConstructor(AstConstructorDeclaration* constructor,
                                  Tuple<AstVariableDeclarator*>& constants,
                                  bool has_instance_initializer)
{
    MethodSymbol* method_symbol = constructor -> constructor_symbol;
    AstMethodBody* constructor_block = constructor -> constructor_body;

    int method_index = methods.NextIndex(); // index for method
    BeginMethod(method_index, method_symbol);

    //
    // Set up the index to account for this, this$0, and normal parameters,
    // so we know where the local variable shadows begin.
    //
    shadow_parameter_offset = unit_type -> EnclosingType() ? 2 : 1;
    if (unit_type -> NumConstructorParameters())
    {
        for (unsigned j = 0; j < method_symbol -> NumFormalParameters(); j++)
            shadow_parameter_offset +=
                GetTypeWords(method_symbol -> FormalParameter(j) -> Type());
    }

    if (control.option.target < JopaOption::SDK1_4)
    {
        //
        // Prior to JDK 1.4, VMs incorrectly complained if shadow
        // initialization happened before the superconstructor, even though
        // the JVMS permits it.
        //
        if (constructor_block -> explicit_constructor_opt)
            EmitStatement(constructor_block -> explicit_constructor_opt);
        // else: Only java.lang.Object has no super() call. During bootstrap,
        // the Object being compiled may differ from control.Object() but
        // its super will point to control.Object() (the stub).
        // For classes with semantic errors that have no explicit_constructor_opt,
        // we just skip generating the super() call - the class file won't be
        // written anyway due to the semantic errors.
    }

    //
    // Supply synthetic field initialization unless constructor calls this().
    // Also initialize all constants.
    //
    if (constructor_block -> explicit_constructor_opt &&
        ! constructor_block -> explicit_constructor_opt -> ThisCallCast())
    {
        if (unit_type -> EnclosingType())
        {
            //
            // Initialize this$0
            //
            VariableSymbol* this0_parameter = unit_type -> EnclosingInstance();
            PutOp(OP_ALOAD_0);
            LoadLocal(1, this0_parameter -> Type());
            PutOp(OP_PUTFIELD);
            PutU2(RegisterFieldref(this0_parameter));
        }

        for (unsigned i = 0, index = shadow_parameter_offset;
             i < unit_type -> NumConstructorParameters(); i++)
        {
            VariableSymbol* shadow = unit_type -> ConstructorParameter(i);
            PutOp(OP_ALOAD_0);
            LoadLocal(index, shadow -> Type());
            PutOp(OP_PUTFIELD);
            if (control.IsDoubleWordType(shadow -> Type()))
                ChangeStack(-1);
            PutU2(RegisterFieldref(shadow));
            index += GetTypeWords(shadow -> Type());
        }

        for (unsigned j = 0; j < constants.Length(); j ++)
            EmitStatement(constants[j]);
    }

    if (control.option.target >= JopaOption::SDK1_4)
    {
        //
        // Since JDK 1.4, VMs correctly allow shadow initialization before
        // the superconstructor, which is necessary to avoid null pointer
        // exceptions with polymorphic calls from the superconstructor.
        //
        if (constructor_block -> explicit_constructor_opt)
            EmitStatement(constructor_block -> explicit_constructor_opt);
        // else: Only java.lang.Object has no super() call. During bootstrap,
        // the Object being compiled may differ from control.Object() but
        // its super will point to control.Object() (the stub).
        // For classes with semantic errors that have no explicit_constructor_opt,
        // we just skip generating the super() call - the class file won't be
        // written anyway due to the semantic errors.
    }

    //
    // Compile instance initializers unless the constructor calls this().
    //
    shadow_parameter_offset = 0;
    if (has_instance_initializer &&
        constructor_block -> explicit_constructor_opt &&
        ! constructor_block -> explicit_constructor_opt -> ThisCallCast())
    {
        PutOp(OP_ALOAD_0);
        PutOp(OP_INVOKESPECIAL);
        CompleteCall(unit_type -> instance_initializer_method, 0);
    }

    EmitBlockStatement(constructor_block);
    EndMethod(method_index, method_symbol);
}


void ByteCode::DeclareField(VariableSymbol* symbol)
{
    int field_index = fields.NextIndex(); // index for field
    fields[field_index] = new FieldInfo();
    const TypeSymbol* type = symbol -> Type();
    if (type -> num_dimensions > 255)
    {
        semantic.ReportSemError(SemanticError::ARRAY_OVERFLOW,
                                symbol -> declarator);
    }

    fields[field_index] -> SetFlags(symbol -> Flags());
    fields[field_index] -> SetNameIndex(RegisterName(symbol ->
                                                     ExternalIdentity()));
    fields[field_index] -> SetDescriptorIndex(RegisterUtf8(type -> signature));

    //
    // Any final field initialized with a constant must have a ConstantValue
    // attribute.  However, the VM only reads this value for static fields.
    //
    if (symbol -> initial_value)
    {
        assert(symbol -> ACC_FINAL());
        assert(type -> Primitive() || type == control.String());
        u2 index = ((control.IsSimpleIntegerValueType(type) ||
                     type == control.boolean_type)
                    ? RegisterInteger(DYNAMIC_CAST<IntLiteralValue*>
                                      (symbol -> initial_value))
                    : type == control.String()
                    ? RegisterString(DYNAMIC_CAST<Utf8LiteralValue*>
                                     (symbol -> initial_value))
                    : type == control.float_type
                    ? RegisterFloat(DYNAMIC_CAST<FloatLiteralValue*>
                                    (symbol -> initial_value))
                    : type == control.long_type
                    ? RegisterLong(DYNAMIC_CAST<LongLiteralValue*>
                                   (symbol -> initial_value))
                    : RegisterDouble(DYNAMIC_CAST<DoubleLiteralValue*>
                                     (symbol -> initial_value)));
        u2 attribute_index = RegisterUtf8(control.ConstantValue_literal);
        fields[field_index] ->
            AddAttribute(new ConstantValueAttribute(attribute_index, index));
    }

    if (symbol -> ACC_SYNTHETIC() &&
        control.option.target < JopaOption::SDK1_5)
    {
        fields[field_index] -> AddAttribute(CreateSyntheticAttribute());
    }

    if (symbol -> IsDeprecated())
        fields[field_index] -> AddAttribute(CreateDeprecatedAttribute());
}


void ByteCode::BeginMethod(int method_index, MethodSymbol* msym)
{
    assert(msym);

#ifdef DUMP
    if (control.option.g)
        Coutput << "(51) Generating code for method \"" << msym -> Name()
                << "\" in "
                << unit_type -> ContainingPackageName() << "/"
                << unit_type -> ExternalName() << endl;
#endif // DUMP
#ifdef JOPA_DEBUG
    if (control.option.debug_trace_stack_change)
        Coutput << endl << "Generating method "
                << unit_type -> ContainingPackageName() << '.'
                << unit_type -> ExternalName() << '.' << msym -> Name()
                << msym -> signature -> value << endl;
#endif // JOPA_DEBUG
    MethodInitialization();

    methods[method_index] = new MethodInfo();
    methods[method_index] ->
        SetNameIndex(RegisterName(msym -> ExternalIdentity()));
    methods[method_index] ->
        SetDescriptorIndex(RegisterUtf8(msym -> signature));
    methods[method_index] -> SetFlags(msym -> Flags());

    if (msym -> ACC_SYNTHETIC() &&
        control.option.target < JopaOption::SDK1_5)
    {
        methods[method_index] -> AddAttribute(CreateSyntheticAttribute());
    }

    if (msym -> IsDeprecated())
        methods[method_index] -> AddAttribute(CreateDeprecatedAttribute());

    //
    // Generate throws attribute if method throws any exceptions
    //
    if (msym -> NumThrows())
    {
        ExceptionsAttribute* exceptions_attribute =
            new ExceptionsAttribute(RegisterUtf8(control.Exceptions_literal));
        for (unsigned i = 0; i < msym -> NumThrows(); i++)
            exceptions_attribute ->
                AddExceptionIndex(RegisterClass(msym -> Throws(i)));
        methods[method_index] -> AddAttribute(exceptions_attribute);
    }

    //
    // Add Signature attribute for generic methods (Java 5+)
    // Methods need signature if they have:
    // 1. Method type parameters (NumTypeParameters() > 0), OR
    // 2. Return type is a class type parameter (return_type_param_index >= 0)
    //
    if (control.option.target >= JopaOption::SDK1_5 &&
        (msym -> NumTypeParameters() > 0 || msym -> return_type_param_index >= 0))
    {
        msym -> SetGenericSignature(control);
        if (msym -> GenericSignature())
        {
            SignatureAttribute* signature_attribute =
                new SignatureAttribute(RegisterUtf8(control.Signature_literal),
                                       RegisterUtf8(msym -> GenericSignature()));
            methods[method_index] -> AddAttribute(signature_attribute);
        }
    }

    //
    // Process RuntimeVisibleAnnotations for methods (Java 5+)
    //
    if (control.option.target >= JopaOption::SDK1_5)
    {
    AstMethodDeclaration* method_decl = msym -> declaration ? msym -> declaration -> MethodDeclarationCast() : NULL;
    AstConstructorDeclaration* constructor_decl = (!method_decl && msym -> declaration) ? msym -> declaration -> ConstructorDeclarationCast() : NULL;
    AstModifiers* mods = method_decl ? method_decl -> modifiers_opt :
                        constructor_decl ? constructor_decl -> modifiers_opt : NULL;

    if (mods)
    {
        AnnotationsAttribute* annotations_attr = NULL;

        for (unsigned i = 0; i < mods -> NumModifiers(); i++)
        {
            AstAnnotation* annotation = mods -> Modifier(i) -> AnnotationCast();
            if (annotation && annotation -> name)
            {
                // Generate annotation with type descriptor and element-value pairs
                LexStream* lex_stream = unit_type -> file_symbol -> lex_stream;
                TokenIndex id_token = annotation -> name -> identifier_token;
                const wchar_t* name_text = lex_stream -> NameString(id_token);
                unsigned name_length = lex_stream -> NameStringLength(id_token);

                // Build descriptor string "Lname;" in wchar_t
                wchar_t* descriptor_text = new wchar_t[name_length + 3];
                descriptor_text[0] = U_L;
                for (unsigned j = 0; j < name_length; j++)
                {
                    descriptor_text[j + 1] = name_text[j];
                }
                descriptor_text[name_length + 1] = U_SEMICOLON;
                descriptor_text[name_length + 2] = U_NULL;

                u2 type_index = RegisterUtf8(control.ConvertUnicodeToUtf8(descriptor_text));
                delete [] descriptor_text;

                if (! annotations_attr)
                {
                    annotations_attr = new AnnotationsAttribute(
                        RegisterUtf8(control.RuntimeVisibleAnnotations_literal), true);
                }

                Annotation* annot = new Annotation(type_index);

                // Process annotation element-value pairs
                for (unsigned j = 0; j < annotation -> NumMemberValuePairs(); j++)
                {
                    AstMemberValuePair* pair = annotation -> MemberValuePair(j);
                    if (pair && pair -> member_value)
                    {
                        // Get the element name (default to "value" for single-element syntax)
                        const wchar_t* element_name;
                        if (pair -> identifier_token_opt)
                        {
                            element_name = lex_stream -> NameString(pair -> identifier_token_opt);
                        }
                        else
                        {
                            element_name = L"value";  // Default for @Anno(val) syntax
                        }
                        u2 element_name_index = RegisterUtf8(control.ConvertUnicodeToUtf8(element_name));

                        // Get the value based on AST node type
                        Ast::AstKind kind = pair -> member_value -> kind;

                        // Handle string literals
                        if (kind == Ast::STRING_LITERAL)
                        {
                            AstStringLiteral* str_lit = (AstStringLiteral*) pair -> member_value;
                            const wchar_t* str_text = lex_stream -> NameString(str_lit -> string_literal_token);
                            unsigned str_length = lex_stream -> NameStringLength(str_lit -> string_literal_token);

                            // Remove quotes from string literal
                            wchar_t* unquoted = new wchar_t[str_length - 1];
                            for (unsigned k = 0; k < str_length - 2; k++)
                            {
                                unquoted[k] = str_text[k + 1];
                            }
                            unquoted[str_length - 2] = U_NULL;

                            u2 string_index = RegisterUtf8(control.ConvertUnicodeToUtf8(unquoted));
                            delete [] unquoted;

                            AnnotationComponentValue* component =
                                new AnnotationComponentConstant(
                                    AnnotationComponentValue::COMPONENT_string,
                                    string_index);
                            annot -> AddComponent(element_name_index, component);
                        }
                        // Handle integer literals
                        else if (kind == Ast::INTEGER_LITERAL)
                        {
                            AstIntegerLiteral* int_lit = (AstIntegerLiteral*) pair -> member_value;
                            const wchar_t* int_text = lex_stream -> NameString(int_lit -> integer_literal_token);

                            // Parse the integer value
                            i4 int_val = 0;
                            for (unsigned k = 0; int_text[k] != U_NULL; k++)
                            {
                                if (int_text[k] >= U_0 && int_text[k] <= U_9)
                                {
                                    int_val = int_val * 10 + (int_text[k] - U_0);
                                }
                            }

                            u2 int_index = RegisterInteger(control.int_pool.FindOrInsert(int_val));
                            AnnotationComponentValue* component =
                                new AnnotationComponentConstant(
                                    AnnotationComponentValue::COMPONENT_int,
                                    int_index);
                            annot -> AddComponent(element_name_index, component);
                        }
                        // Handle other primitive literals
                        else if (kind == Ast::LONG_LITERAL)
                        {
                            AstLongLiteral* long_lit = (AstLongLiteral*) pair -> member_value;
                            const wchar_t* long_text = lex_stream -> NameString(long_lit -> long_literal_token);

                            // Parse the long value (simplified - skip 'L' suffix)
                            LongInt long_val = 0;
                            for (unsigned k = 0; long_text[k] != U_NULL && long_text[k] != U_L && long_text[k] != U_l; k++)
                            {
                                if (long_text[k] >= U_0 && long_text[k] <= U_9)
                                {
                                    long_val = LongInt(long_val * 10 + (long_text[k] - U_0));
                                }
                            }

                            u2 long_index = RegisterLong(control.long_pool.FindOrInsert(long_val));
                            AnnotationComponentValue* component =
                                new AnnotationComponentConstant(
                                    AnnotationComponentValue::COMPONENT_long,
                                    long_index);
                            annot -> AddComponent(element_name_index, component);
                        }
                        else if (kind == Ast::TRUE_LITERAL || kind == Ast::FALSE_LITERAL)
                        {
                            i4 bool_val = (kind == Ast::TRUE_LITERAL) ? 1 : 0;
                            u2 bool_index = RegisterInteger(control.int_pool.FindOrInsert(bool_val));
                            AnnotationComponentValue* component =
                                new AnnotationComponentConstant(
                                    AnnotationComponentValue::COMPONENT_boolean,
                                    bool_index);
                            annot -> AddComponent(element_name_index, component);
                        }
                        else if (kind == Ast::CHARACTER_LITERAL)
                        {
                            AstCharacterLiteral* char_lit = (AstCharacterLiteral*) pair -> member_value;
                            const wchar_t* char_text = lex_stream -> NameString(char_lit -> character_literal_token);

                            // Extract character value (between single quotes)
                            i4 char_val = 0;
                            if (char_text[1] == U_BACKSLASH)
                            {
                                // Handle escape sequences
                                if (char_text[2] == U_n) char_val = U_LINE_FEED;
                                else if (char_text[2] == U_r) char_val = U_CARRIAGE_RETURN;
                                else if (char_text[2] == U_t) char_val = U_HORIZONTAL_TAB;
                                else char_val = char_text[2];
                            }
                            else
                            {
                                char_val = char_text[1];
                            }

                            u2 char_index = RegisterInteger(control.int_pool.FindOrInsert(char_val));
                            AnnotationComponentValue* component =
                                new AnnotationComponentConstant(
                                    AnnotationComponentValue::COMPONENT_char,
                                    char_index);
                            annot -> AddComponent(element_name_index, component);
                        }
                        // Handle class literals (e.g., String.class)
                        else if (kind == Ast::CLASS_LITERAL)
                        {
                            AstClassLiteral* class_lit = (AstClassLiteral*) pair -> member_value;
                            if (class_lit -> type)
                            {
                                // Try to get from symbol first
                                if (class_lit -> type -> symbol)
                                {
                                    TypeSymbol* type_sym = class_lit -> type -> symbol;
                                    const char* type_name_utf8 = type_sym -> fully_qualified_name -> value;
                                    unsigned type_name_len = type_sym -> fully_qualified_name -> length;

                                    wchar_t* type_name_unicode = new wchar_t[type_name_len + 1];
                                    int unicode_len = Control::ConvertUtf8ToUnicode(type_name_unicode, type_name_utf8, type_name_len);

                                    // Build descriptor "Lpackage/Type;"
                                    wchar_t* type_descriptor = new wchar_t[unicode_len + 3];
                                    type_descriptor[0] = U_L;
                                    for (int k = 0; k < unicode_len; k++)
                                    {
                                        type_descriptor[k + 1] = (type_name_unicode[k] == U_DOT) ? U_SLASH : type_name_unicode[k];
                                    }
                                    type_descriptor[unicode_len + 1] = U_SEMICOLON;
                                    type_descriptor[unicode_len + 2] = U_NULL;

                                    u2 class_index = RegisterUtf8(control.ConvertUnicodeToUtf8(type_descriptor));
                                    delete [] type_name_unicode;
                                    delete [] type_descriptor;

                                    AnnotationComponentValue* component =
                                        new AnnotationComponentConstant(
                                            AnnotationComponentValue::COMPONENT_class,
                                            class_index);
                                    annot -> AddComponent(element_name_index, component);
                                }
                                // Fall back to extracting from AST tokens
                                else if (class_lit -> type -> kind == Ast::TYPE)
                                {
                                    AstTypeName* type_name_node = (AstTypeName*) class_lit -> type;
                                    if (type_name_node -> name)
                                    {
                                        TokenIndex type_token = type_name_node -> name -> identifier_token;
                                        const wchar_t* type_name = lex_stream -> NameString(type_token);
                                        unsigned type_len = lex_stream -> NameStringLength(type_token);

                                        // Build descriptor "Ljava/lang/TypeName;"
                                        wchar_t* type_descriptor = new wchar_t[type_len + 3];
                                        type_descriptor[0] = U_L;
                                        for (unsigned k = 0; k < type_len; k++)
                                        {
                                            type_descriptor[k + 1] = type_name[k];
                                        }
                                        type_descriptor[type_len + 1] = U_SEMICOLON;
                                        type_descriptor[type_len + 2] = U_NULL;

                                        u2 class_index = RegisterUtf8(control.ConvertUnicodeToUtf8(type_descriptor));
                                        delete [] type_descriptor;

                                        AnnotationComponentValue* component =
                                            new AnnotationComponentConstant(
                                                AnnotationComponentValue::COMPONENT_class,
                                                class_index);
                                        annot -> AddComponent(element_name_index, component);
                                    }
                                }
                            }
                        }
                        // Handle nested annotations
                        else if (kind == Ast::ANNOTATION)
                        {
                            AstAnnotation* nested_annotation = (AstAnnotation*) pair -> member_value;
                            if (nested_annotation -> name)
                            {
                                // Build nested annotation type descriptor
                                TokenIndex nested_id_token = nested_annotation -> name -> identifier_token;
                                const wchar_t* nested_name_text = lex_stream -> NameString(nested_id_token);
                                unsigned nested_name_length = lex_stream -> NameStringLength(nested_id_token);

                                wchar_t* nested_descriptor_text = new wchar_t[nested_name_length + 3];
                                nested_descriptor_text[0] = U_L;
                                for (unsigned k = 0; k < nested_name_length; k++)
                                {
                                    nested_descriptor_text[k + 1] = nested_name_text[k];
                                }
                                nested_descriptor_text[nested_name_length + 1] = U_SEMICOLON;
                                nested_descriptor_text[nested_name_length + 2] = U_NULL;

                                u2 nested_type_index = RegisterUtf8(control.ConvertUnicodeToUtf8(nested_descriptor_text));
                                delete [] nested_descriptor_text;

                                Annotation* nested_annot = new Annotation(nested_type_index);

                                // Recursively process nested annotation parameters
                                // (This is a simplified version - full recursion would need helper function)

                                AnnotationComponentValue* component =
                                    new AnnotationComponentAnnotation(nested_annot);
                                annot -> AddComponent(element_name_index, component);
                            }
                        }
                        // Handle array initializers
                        else if (kind == Ast::ARRAY_INITIALIZER)
                        {
                            AstArrayInitializer* array_init = (AstArrayInitializer*) pair -> member_value;
                            AnnotationComponentArray* array_component = new AnnotationComponentArray();

                            for (unsigned k = 0; k < array_init -> NumVariableInitializers(); k++)
                            {
                                AstMemberValue* init_value = array_init -> VariableInitializer(k);
                                Ast::AstKind init_kind = init_value -> kind;

                                // Handle array element types
                                if (init_kind == Ast::STRING_LITERAL)
                                {
                                    AstStringLiteral* str_lit = (AstStringLiteral*) init_value;
                                    const wchar_t* str_text = lex_stream -> NameString(str_lit -> string_literal_token);
                                    unsigned str_length = lex_stream -> NameStringLength(str_lit -> string_literal_token);

                                    wchar_t* unquoted = new wchar_t[str_length - 1];
                                    for (unsigned m = 0; m < str_length - 2; m++)
                                    {
                                        unquoted[m] = str_text[m + 1];
                                    }
                                    unquoted[str_length - 2] = U_NULL;

                                    u2 string_index = RegisterUtf8(control.ConvertUnicodeToUtf8(unquoted));
                                    delete [] unquoted;

                                    array_component -> AddValue(
                                        new AnnotationComponentConstant(
                                            AnnotationComponentValue::COMPONENT_string,
                                            string_index));
                                }
                                else if (init_kind == Ast::INTEGER_LITERAL)
                                {
                                    AstIntegerLiteral* int_lit = (AstIntegerLiteral*) init_value;
                                    const wchar_t* int_text = lex_stream -> NameString(int_lit -> integer_literal_token);

                                    i4 int_val = 0;
                                    for (unsigned m = 0; int_text[m] != U_NULL; m++)
                                    {
                                        if (int_text[m] >= U_0 && int_text[m] <= U_9)
                                        {
                                            int_val = int_val * 10 + (int_text[m] - U_0);
                                        }
                                    }

                                    array_component -> AddValue(
                                        new AnnotationComponentConstant(
                                            AnnotationComponentValue::COMPONENT_int,
                                            RegisterInteger(control.int_pool.FindOrInsert(int_val))));
                                }
                            }

                            annot -> AddComponent(element_name_index, array_component);
                        }
                        // Handle enum constants (field access like Priority.HIGH)
                        else if (kind == Ast::DOT)
                        {
                            AstFieldAccess* field_access = (AstFieldAccess*) pair -> member_value;

                            // Get the enum type from the base expression
                            AstExpression* base_expr = field_access -> base;
                            if (base_expr && base_expr -> symbol)
                            {
                                TypeSymbol* enum_type = base_expr -> symbol -> TypeCast();
                                if (enum_type)
                                {
                                    // Build enum type descriptor "Lpackage/EnumType;" from fully_qualified_name
                                    // First convert UTF8 to Unicode
                                    const char* type_name_utf8 = enum_type -> fully_qualified_name -> value;
                                    unsigned type_name_len = enum_type -> fully_qualified_name -> length;

                                    wchar_t* type_name_unicode = new wchar_t[type_name_len + 1];
                                    int unicode_len = Control::ConvertUtf8ToUnicode(type_name_unicode, type_name_utf8, type_name_len);

                                    // Build descriptor "Lpackage/Type;" with slashes instead of dots
                                    wchar_t* type_descriptor = new wchar_t[unicode_len + 3];
                                    type_descriptor[0] = U_L;
                                    for (int k = 0; k < unicode_len; k++)
                                    {
                                        type_descriptor[k + 1] = (type_name_unicode[k] == U_DOT) ? U_SLASH : type_name_unicode[k];
                                    }
                                    type_descriptor[unicode_len + 1] = U_SEMICOLON;
                                    type_descriptor[unicode_len + 2] = U_NULL;

                                    u2 type_index = RegisterUtf8(control.ConvertUnicodeToUtf8(type_descriptor));
                                    delete [] type_name_unicode;
                                    delete [] type_descriptor;

                                    // Get the enum constant name
                                    const wchar_t* const_name = lex_stream -> NameString(field_access -> identifier_token);
                                    u2 const_index = RegisterUtf8(control.ConvertUnicodeToUtf8(const_name));

                                    AnnotationComponentValue* component =
                                        new AnnotationComponentEnum(type_index, const_index);
                                    annot -> AddComponent(element_name_index, component);
                                }
                            }
                        }
                    }
                }

                annotations_attr -> AddAnnotation(annot);
            }
        }

        if (annotations_attr)
        {
            methods[method_index] -> AddAttribute(annotations_attr);
        }
    }
    } // end SDK1_5+ annotations guard

    //
    // here if need code and associated attributes.
    //
    if (! (msym -> ACC_ABSTRACT() || msym -> ACC_NATIVE()))
    {
        // Increase stack depth to account for inlined finally blocks (Java 7+)
        // Semantic analysis calculates static nesting depth, but inlining increases
        // effective nesting depth during code generation.
        unsigned stack_depth = msym -> max_block_depth + 64;
        method_stack =
            new MethodStack(stack_depth,
                            msym -> block_symbol -> max_variable_index);
        code_attribute =
            new CodeAttribute(RegisterUtf8(control.Code_literal),
                              msym -> block_symbol -> max_variable_index);
        line_number = 0;
        line_number_table_attribute = new LineNumberTableAttribute
            (RegisterUtf8(control.LineNumberTable_literal));

        local_variable_table_attribute = (control.option.g & JopaOption::VARS)
            ? (new LocalVariableTableAttribute
               (RegisterUtf8(control.LocalVariableTable_literal)))
            : (LocalVariableTableAttribute*) NULL;

        // Initialize StackMapGenerator for Java 7+ bytecode
        if (control.option.target >= JopaOption::SDK1_7)
        {
            stack_map_generator = new StackMapGenerator(control, *this);
            stack_map_generator->InitializeMethod(msym, unit_type);
        }
        else
        {
            stack_map_generator = NULL;
        }
    }

    if (msym -> Type() -> num_dimensions > 255)
    {
        assert(msym -> declaration -> MethodDeclarationCast());
        Ast* type = ((AstMethodDeclaration*) msym -> declaration) -> type;

        semantic.ReportSemError(SemanticError::ARRAY_OVERFLOW, type);
    }

    Coutput << "[STACKMAP DEBUG] BeginMethod: Checking parameters for " << msym->Name() 
            << " ptr=" << (unsigned long)msym 
            << " IsTyped=" << msym->IsTyped() << endl;

    VariableSymbol* parameter = NULL;
    if (msym->IsTyped()) // Added check to avoid crash when type_ is null
    {
        for (unsigned i = 0; i < msym -> NumFormalParameters(); i++)
        {
            parameter = msym -> FormalParameter(i);
            if (parameter -> Type() -> num_dimensions > 255)
            {
                semantic.ReportSemError(SemanticError::ARRAY_OVERFLOW,
                                        parameter -> declarator);
            }
        }
    }
    if (parameter)
    {
        int last_parameter_index = parameter -> LocalVariableIndex();
        if (control.IsDoubleWordType(parameter -> Type()))
            last_parameter_index++;
        if (last_parameter_index >= 255)
        {
            assert(msym -> declaration);

            AstMethodDeclaration* method_declaration =
                msym -> declaration -> MethodDeclarationCast();
            AstConstructorDeclaration* constructor_declaration =
                msym -> declaration -> ConstructorDeclarationCast();
            AstMethodDeclarator* declarator = method_declaration
                ? method_declaration -> method_declarator
                : constructor_declaration -> constructor_declarator;

            semantic.ReportSemError(SemanticError::PARAMETER_OVERFLOW,
                                    declarator -> left_parenthesis_token,
                                    declarator -> right_parenthesis_token,
                                    msym -> Header(),
                                    unit_type -> ContainingPackageName(),
                                    unit_type -> ExternalName());
        }
    }
}


void ByteCode::EndMethod(int method_index, MethodSymbol* msym)
{
    assert(msym);

    if (! (msym -> ACC_ABSTRACT() || msym -> ACC_NATIVE()))
    {
        //
        // Make sure that no component in the code attribute exceeded its
        // limit.
        //
        if (msym -> block_symbol -> max_variable_index > 65535)
        {
            semantic.ReportSemError(SemanticError::LOCAL_VARIABLES_OVERFLOW,
                                    msym -> declaration, msym -> Header(),
                                    unit_type -> ContainingPackageName(),
                                    unit_type -> ExternalName());
        }

        if (max_stack > 65535)
        {
            semantic.ReportSemError(SemanticError::STACK_OVERFLOW,
                                    msym -> declaration, msym -> Header(),
                                    unit_type -> ContainingPackageName(),
                                    unit_type -> ExternalName());
        }

        if (code_attribute -> CodeLengthExceeded())
        {
            semantic.ReportSemError(SemanticError::CODE_OVERFLOW,
                                    msym -> declaration, msym -> Header(),
                                    unit_type -> ContainingPackageName(),
                                    unit_type -> ExternalName());
        }

        //
        //
        //
        code_attribute -> SetMaxStack(max_stack);

        //
        // Sanity check - make sure nothing jumped past here
        // Note: last_label_pc == CodeLength() is valid when a label is defined
        // at the end of a method (e.g., after exhaustive switch or unreachable code)
        //
        assert((u2) last_label_pc <= code_attribute -> CodeLength() ||
               code_attribute -> CodeLength() == 0x0ffff);
        assert(stack_depth == 0);

        //
        // attribute length:
        // Need to review how to make attribute_name and attribute_length.
        // Only write line number attribute if there are line numbers to
        // write, and -g:lines is enabled.
        //
        if ((control.option.g & JopaOption::LINES) &&
            line_number_table_attribute -> LineNumberTableLength())
        {
             code_attribute -> AddAttribute(line_number_table_attribute);
        }
        else
        {
            // line_number_table_attribute not needed, so delete it now
            delete line_number_table_attribute;
        }

        //
        // Debug level -g:vars & not dealing with generated accessed method
        //
        if ((control.option.g & JopaOption::VARS)
            && (! msym -> accessed_member)
            && (msym -> Identity() != control.class_name_symbol))
        {
            if (! msym -> ACC_STATIC()) // add 'this' to local variable table
            {
                local_variable_table_attribute ->
                    AddLocalVariable(0, code_attribute -> CodeLength(),
                                     RegisterUtf8(control.this_name_symbol -> Utf8_literal),
                                     RegisterUtf8(msym -> containing_type -> signature),
                                     0);
            }

            //
            // For a normal constructor or method.
            //
            for (unsigned i = 0; i < msym -> NumFormalParameters(); i++)
            {
                VariableSymbol* parameter = msym -> FormalParameter(i);
                int local_index = parameter -> LocalVariableIndex();
                // Skip parameters without valid local variable indices
                if (local_index < 0)
                    continue;
                local_variable_table_attribute ->
                    AddLocalVariable(0, code_attribute -> CodeLength(),
                                     RegisterName(parameter -> ExternalIdentity()),
                                     RegisterUtf8(parameter -> Type() -> signature),
                                     (u2) local_index);
            }

            if (local_variable_table_attribute -> LocalVariableTableLength())
                 code_attribute -> AddAttribute(local_variable_table_attribute);
            else
                // local_variable_table_attribute not needed, so delete it now
                delete local_variable_table_attribute;
        }
        else delete local_variable_table_attribute;

        //
        // Generate StackMapTable for Java 7+ bytecode
        //
        if (stack_map_generator)
        {
            StackMapTableAttribute* stack_map_attr =
                stack_map_generator->GenerateAttribute(
                    RegisterUtf8(control.StackMapTable_literal));
            if (stack_map_attr)
            {
                code_attribute->AddAttribute(stack_map_attr);
            }
            delete stack_map_generator;
            stack_map_generator = NULL;
        }

        methods[method_index] -> AddAttribute(code_attribute);

        delete method_stack;
    }
}


//
// This is called to initialize non-constant static fields, and all instance
// fields, that were declared with optional initializers.
//
void ByteCode::InitializeVariable(AstVariableDeclarator* vd)
{
    assert(vd -> variable_initializer_opt && vd -> symbol);

    AstExpression* expression =
        vd -> variable_initializer_opt -> ExpressionCast();
    if (expression)
    {
        if (vd -> symbol -> ACC_STATIC())
            assert(! vd -> symbol -> initial_value);
        else
            PutOp(OP_ALOAD_0); // load 'this' for instance variables
        EmitExpression(expression);
    }
    else
    {
        AstArrayInitializer* array_initializer =
            vd -> variable_initializer_opt -> ArrayInitializerCast();
        assert(array_initializer);
        if (! vd -> symbol -> ACC_STATIC())
            PutOp(OP_ALOAD_0); // load 'this' for instance variables
        InitializeArray(vd -> symbol -> Type(), array_initializer);
    }

    PutOp(vd -> symbol -> ACC_STATIC() ? OP_PUTSTATIC : OP_PUTFIELD);
    if (expression && control.IsDoubleWordType(expression -> Type()))
        ChangeStack(-1);
    PutU2(RegisterFieldref(vd -> symbol));
}


void ByteCode::InitializeArray(const TypeSymbol* type,
                               AstArrayInitializer* array_initializer,
                               bool need_value)
{
    TypeSymbol* subtype = type -> ArraySubtype();

    if (need_value)
    {
        LoadImmediateInteger(array_initializer -> NumVariableInitializers());
        EmitNewArray(1, type); // make the array
    }
    for (unsigned i = 0;
         i < array_initializer -> NumVariableInitializers(); i++)
    {
        Ast* entry = array_initializer -> VariableInitializer(i);
        AstExpression* expr = entry -> ExpressionCast();
        if (expr && (IsZero(expr) || expr -> Type() == control.null_type))
        {
            bool optimize;
            if (expr -> Type() == control.float_type)
            {
                FloatLiteralValue* value = DYNAMIC_CAST<FloatLiteralValue*>
                    (expr -> value);
                optimize = value -> value.IsPositiveZero();
            }
            else if (expr -> Type() == control.double_type)
            {
                DoubleLiteralValue* value = DYNAMIC_CAST<DoubleLiteralValue*>
                    (expr -> value);
                optimize = value -> value.IsPositiveZero();
            }
            else optimize = true;
            if (optimize)
            {
                EmitExpression(expr, false);
                continue;
            }
        }

        if (need_value)
        {
            PutOp(OP_DUP);
            LoadImmediateInteger(i);
        }
        if (expr)
             EmitExpression(expr, need_value);
        else
        {
            assert(entry -> ArrayInitializerCast());
            InitializeArray(subtype, entry -> ArrayInitializerCast(),
                            need_value);
        }
        if (need_value)
            StoreArrayElement(subtype);
    }
}


//
// Generate code for local variable declaration.
//
void ByteCode::DeclareLocalVariable(AstVariableDeclarator* declarator)
{
    if (control.option.g & JopaOption::VARS)
    {
#ifdef JOPA_DEBUG
        // Must be uninitialized.
        assert(method_stack -> StartPc(declarator -> symbol) == 0xFFFF);
#endif // JOPA_DEBUG
#ifdef DUMP
        Coutput << "(53) Variable \"" << declarator -> symbol -> Name()
                << "\" numbered "
                << declarator -> symbol -> LocalVariableIndex()
                << " was processed" << endl;
#endif // DUMP
        method_stack -> StartPc(declarator -> symbol) =
            code_attribute -> CodeLength();
    }

    TypeSymbol* type = declarator -> symbol -> Type();
    if (type -> num_dimensions > 255)
        semantic.ReportSemError(SemanticError::ARRAY_OVERFLOW, declarator);

    if (declarator -> symbol -> initial_value)
    {
        //
        // Optimization: If we are not tracking local variable names, we do
        // not need to waste space on a constant as it is always inlined.
        //
        if (! (control.option.g & JopaOption::VARS))
            return;
        LoadLiteral(declarator -> symbol -> initial_value,
                    declarator -> symbol -> Type());
    }
    else if (declarator -> variable_initializer_opt)
    {
        AstArrayCreationExpression* ace = declarator ->
            variable_initializer_opt -> ArrayCreationExpressionCast();
        AstArrayInitializer* ai = declarator ->
            variable_initializer_opt -> ArrayInitializerCast();
        if (ace)
            EmitArrayCreationExpression(ace);
        else if (ai)
            InitializeArray(type, ai);
        else // evaluation as expression
        {
            AstExpression* expr =
                (AstExpression*) declarator -> variable_initializer_opt;
            assert(declarator -> variable_initializer_opt -> ExpressionCast());
            EmitExpression(expr);

            // Java 5: Insert boxing/unboxing conversion if needed
            TypeSymbol* expr_type = expr -> Type();
            if (expr_type != type)
                EmitCast(type, expr_type);

            //
            // Prior to JDK 1.5, VMs incorrectly complained if assigning an
            // array type into an element of a null expression (in other
            // words, null was not being treated as compatible with a
            // multi-dimensional array on the aastore opcode).  The
            // workaround requires a checkcast any time null might be
            // assigned to a multi-dimensional local variable or directly
            // used as an array access base.
            //
            if (control.option.target < JopaOption::SDK1_5 &&
                IsMultiDimensionalArray(type) &&
                (StripNops(expr) -> Type() == control.null_type))
            {
                PutOp(OP_CHECKCAST);
                PutU2(RegisterClass(type));
            }
        }
    }
    else return; // if nothing to initialize

    StoreLocal(declarator -> symbol -> LocalVariableIndex(), type);
}


//
// JLS Chapter 13: Blocks and Statements
//  Statements control the sequence of evaluation of Java programs,
//  are executed for their effects and do not have values.
//
// Processing of loops requires a loop stack, especially to handle
// break and continue statements.
// Loops have three labels, LABEL_BEGIN for start of loop body,
// LABEL_BREAK to leave the loop, and LABEL_CONTINUE to continue the iteration.
// Each loop requires a break label; other labels are defined and used
// as needed.
// Labels allocated but never used incur no extra cost in the generated
// byte code, only in additional execution expense during compilation.
//
// This method returns true if the statement is guaranteed to complete
// abruptly (break, continue, throw, return, and special cases of if); it
// allows some dead code elimination.

} // Close namespace Jopa block
