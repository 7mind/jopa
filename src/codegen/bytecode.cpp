#include "bytecode.h"
#include "ast.h"
#include "class.h"
#include "control.h"
#include "semantic.h"
#include "stream.h"
#include "symbol.h"
#include "table.h"
#include "option.h"


namespace Jopa { // Open namespace Jopa block
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
        else
            assert(unit_type == control.Object() &&
                   "A constructor without an explicit constructor invocation");
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
        else
            assert(unit_type == control.Object() &&
                   "A constructor without an explicit constructor invocation");
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
    // Add Signature attribute for generic methods
    //
    if (msym -> NumTypeParameters() > 0)
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
    // Process RuntimeVisibleAnnotations for methods
    //
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

    //
    // here if need code and associated attributes.
    //
    if (! (msym -> ACC_ABSTRACT() || msym -> ACC_NATIVE()))
    {
        method_stack =
            new MethodStack(msym -> max_block_depth,
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
    }

    if (msym -> Type() -> num_dimensions > 255)
    {
        assert(msym -> declaration -> MethodDeclarationCast());
        Ast* type = ((AstMethodDeclaration*) msym -> declaration) -> type;

        semantic.ReportSemError(SemanticError::ARRAY_OVERFLOW, type);
    }

    VariableSymbol* parameter = NULL;
    for (unsigned i = 0; i < msym -> NumFormalParameters(); i++)
    {
        parameter = msym -> FormalParameter(i);
        if (parameter -> Type() -> num_dimensions > 255)
        {
            semantic.ReportSemError(SemanticError::ARRAY_OVERFLOW,
                                    parameter -> declarator);
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
        //
        assert((u2) last_label_pc < code_attribute -> CodeLength() ||
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
                local_variable_table_attribute ->
                    AddLocalVariable(0, code_attribute -> CodeLength(),
                                     RegisterName(parameter -> ExternalIdentity()),
                                     RegisterUtf8(parameter -> Type() -> signature),
                                     parameter -> LocalVariableIndex());
            }

            if (local_variable_table_attribute -> LocalVariableTableLength())
                 code_attribute -> AddAttribute(local_variable_table_attribute);
            else
                // local_variable_table_attribute not needed, so delete it now
                delete local_variable_table_attribute;
        }
        else delete local_variable_table_attribute;

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
//
bool ByteCode::EmitStatement(AstStatement* statement)
{
    if (! statement -> BlockCast())
    {
        line_number_table_attribute ->
            AddLineNumber(code_attribute -> CodeLength(),
                          semantic.lex_stream -> Line(statement -> LeftToken()));
    }

    assert(stack_depth == 0); // stack empty at start of statement

    switch (statement -> kind)
    {
    case Ast::METHOD_BODY:
    case Ast::BLOCK: // JLS 14.2
        return EmitBlockStatement((AstBlock*) statement);
    case Ast::LOCAL_VARIABLE_DECLARATION: // JLS 14.3
        {
            AstLocalVariableStatement* lvs =
                statement -> LocalVariableStatementCast();
            for (unsigned i = 0; i < lvs -> NumVariableDeclarators(); i++)
                DeclareLocalVariable(lvs -> VariableDeclarator(i));
        }
        return false;
    case Ast::EMPTY_STATEMENT: // JLS 14.5
        return false;
    case Ast::EXPRESSION_STATEMENT: // JLS 14.7
        EmitStatementExpression(statement -> ExpressionStatementCast() ->
                                expression);
        return false;
    case Ast::IF: // JLS 14.8
        {
            AstIfStatement* if_statement = (AstIfStatement*) statement;
            // Constant condition.
            if (IsOne(if_statement -> expression))
                return EmitBlockStatement(if_statement -> true_statement);
            if (IsZero(if_statement -> expression))
            {
                if (if_statement -> false_statement_opt)
                    return EmitBlockStatement(if_statement ->
                                              false_statement_opt);
                return false;
            }
            // True and false parts.
            if (if_statement -> false_statement_opt &&
                ! IsNop(if_statement -> false_statement_opt))
            {
                if (IsNop(if_statement -> true_statement))
                {
                    Label label;
                    EmitBranchIfExpression(if_statement -> expression,
                                           true, label,
                                           (if_statement ->
                                            false_statement_opt));
                    assert(stack_depth == 0);
                    EmitBlockStatement(if_statement -> false_statement_opt);
                    DefineLabel(label);
                    CompleteLabel(label);
                    return false;
                }
                Label label1,
                      label2;
                bool abrupt;
                AstBlock* true_statement = if_statement -> true_statement;
                EmitBranchIfExpression(if_statement -> expression,
                                       false, label1, true_statement);
                assert(stack_depth == 0);

                abrupt = EmitBlockStatement(true_statement);
                if (! abrupt)
                    EmitBranch(OP_GOTO, label2,
                               if_statement -> false_statement_opt);

                DefineLabel(label1);
                abrupt &= EmitBlockStatement(if_statement ->
                                             false_statement_opt);

                if (! abrupt)
                {
                    DefineLabel(label2);
                    CompleteLabel(label2);
                }
                CompleteLabel(label1);
                return abrupt;
            }
            // No false part.
            if (IsNop(if_statement -> true_statement))
            {
                EmitExpression(if_statement -> expression, false);
                return false;
            }
            Label label1;
            EmitBranchIfExpression(if_statement -> expression,
                                   false, label1,
                                   if_statement -> true_statement);
            assert(stack_depth == 0);
            EmitBlockStatement(if_statement -> true_statement);
            DefineLabel(label1);
            CompleteLabel(label1);
            return false;
        }
    case Ast::SWITCH: // JLS 14.9
        return EmitSwitchStatement(statement -> SwitchStatementCast());
    case Ast::SWITCH_BLOCK: // JLS 14.9
    case Ast::SWITCH_LABEL:
        //
        // These nodes are handled by SwitchStatement and
        // are not directly visited.
        //
        assert(false && "faulty logic encountered");
        return false;
    case Ast::WHILE: // JLS 14.10
        {
            AstWhileStatement* wp = statement -> WhileStatementCast();
            bool abrupt = false;
            //
            // Branch to continuation test. This test is placed after the
            // body of the loop we can fall through into it after each
            // loop iteration without the need for an additional branch,
            // unless the loop body always completes abruptly.
            //
            if (! wp -> statement -> can_complete_normally)
            {
                if (wp -> expression -> IsConstant())
                {
                    // must be true, or internal statement would be
                    // unreachable
                    assert(semantic.IsConstantTrue(wp -> expression));
                    abrupt = true;
                }
                else
                {
                    line_number_table_attribute ->
                        AddLineNumber(code_attribute -> CodeLength(),
                                      semantic.lex_stream -> Line(wp -> expression -> LeftToken()));
                    EmitBranchIfExpression(wp -> expression, false,
                                           method_stack -> TopBreakLabel(),
                                           wp -> statement);
                }
                EmitBlockStatement(wp -> statement);
                assert(stack_depth == 0);
                return abrupt;
            }
            Label& continue_label = method_stack -> TopContinueLabel();
            if (wp -> expression -> IsConstant())
            {
                // must be true, or internal statement would be
                // unreachable
                assert(semantic.IsConstantTrue(wp -> expression));
                abrupt = true;
            }
            else
                EmitBranch(OP_GOTO, continue_label, wp -> statement);
            Label begin_label;
            DefineLabel(begin_label);
            u2 begin_pc = code_attribute -> CodeLength();
            abrupt |= EmitBlockStatement(wp -> statement);
            bool empty = (begin_pc == code_attribute -> CodeLength());
            DefineLabel(continue_label);
            assert(stack_depth == 0);

            //
            // Reset the line number before evaluating the expression
            //
            line_number_table_attribute ->
                AddLineNumber(code_attribute -> CodeLength(),
                              semantic.lex_stream -> Line(wp -> expression -> LeftToken()));

            EmitBranchIfExpression(wp -> expression, true,
                                   empty ? continue_label : begin_label,
                                   wp -> statement);
            CompleteLabel(begin_label);
            CompleteLabel(continue_label);
            return abrupt && ! wp -> can_complete_normally;
        }
    case Ast::DO: // JLS 14.11
        {
            AstDoStatement* sp = statement -> DoStatementCast();
            Label begin_label;
            DefineLabel(begin_label);
            bool abrupt = EmitBlockStatement(sp -> statement);
            if (IsLabelUsed(method_stack -> TopContinueLabel()))
            {
                DefineLabel(method_stack -> TopContinueLabel());
                CompleteLabel(method_stack -> TopContinueLabel());
                abrupt = false;
            }
            assert(stack_depth == 0);

            if (! abrupt)
            {
                //
                // Reset the line number before evaluating the expression
                //
                line_number_table_attribute ->
                    AddLineNumber(code_attribute -> CodeLength(),
                                  semantic.lex_stream -> Line(sp -> expression -> LeftToken()));
                EmitBranchIfExpression(sp -> expression, true,
                                       begin_label, sp -> statement);
            }
            CompleteLabel(begin_label);
            return (abrupt || IsOne(sp -> expression)) &&
                ! sp -> can_complete_normally;
        }
    case Ast::FOR: // JLS 14.12
        {
            AstForStatement* for_statement = statement -> ForStatementCast();
            bool abrupt = false;
            for (unsigned i = 0; i < for_statement -> NumForInitStatements(); i++)
                EmitStatement(for_statement -> ForInitStatement(i));
            Label begin_label;
            Label test_label;
            //
            // The loop test is placed after the body, unless the body
            // always completes abruptly, to save an additional jump.
            //
            if (! for_statement -> statement -> can_complete_normally)
            {
                abrupt = true;
                if (for_statement -> end_expression_opt)
                {
                    if (for_statement -> end_expression_opt -> IsConstant())
                    {
                        // must be true, or internal statement would be
                        // unreachable
                        assert(semantic.IsConstantTrue(for_statement -> end_expression_opt));
                    }
                    else
                    {
                        abrupt = false;
                        line_number_table_attribute ->
                            AddLineNumber(code_attribute -> CodeLength(),
                                          semantic.lex_stream -> Line(for_statement -> end_expression_opt -> LeftToken()));
                        EmitBranchIfExpression(for_statement -> end_expression_opt,
                                               false,
                                               method_stack -> TopBreakLabel(),
                                               for_statement -> statement);
                    }
                }
                EmitBlockStatement(for_statement -> statement);
                assert(stack_depth == 0);
                return abrupt;
            }
            Label& continue_label = method_stack -> TopContinueLabel();
            if (for_statement -> end_expression_opt &&
                ! for_statement -> end_expression_opt -> IsConstant())
            {
                EmitBranch(OP_GOTO,
                           (for_statement -> NumForUpdateStatements()
                            ? test_label : continue_label),
                           for_statement -> statement);
            }
            else
                abrupt = true;
            DefineLabel(begin_label);
            u2 begin_pc = code_attribute -> CodeLength();
            abrupt |= EmitBlockStatement(for_statement -> statement);
            bool empty = (begin_pc == code_attribute -> CodeLength());
            DefineLabel(continue_label);
            for (unsigned j = 0;
                 j < for_statement -> NumForUpdateStatements(); j++)
            {
                EmitStatement(for_statement -> ForUpdateStatement(j));
            }
            DefineLabel(test_label);
            CompleteLabel(test_label);

            AstExpression* end_expr = for_statement -> end_expression_opt;
            if (end_expr)
            {
                assert(stack_depth == 0);

                //
                // Reset the line number before evaluating the expression
                //
                line_number_table_attribute ->
                    AddLineNumber(code_attribute -> CodeLength(),
                                  semantic.lex_stream -> Line(end_expr ->
                                                              LeftToken()));

                EmitBranchIfExpression(end_expr, true,
                                       empty ? continue_label : begin_label,
                                       for_statement -> statement);
            }
            else EmitBranch(OP_GOTO, empty ? continue_label : begin_label,
                            for_statement -> statement);
            CompleteLabel(continue_label);
            CompleteLabel(begin_label);
            return abrupt && ! for_statement -> can_complete_normally;
        }
    case Ast::FOREACH: // JSR 201
        EmitForeachStatement((AstForeachStatement*) statement);
        return false;
    case Ast::BREAK: // JLS 14.13
        {
            unsigned nesting_level =
                statement -> BreakStatementCast() -> nesting_level;
            AstBlock* over = method_stack -> Block(nesting_level);
            u2 jump_size = (over -> RightToken() - over -> LeftToken() <
                            TOKEN_WIDTH_REQUIRING_GOTOW) ? 3 : 5;
            if (ProcessAbruptExit(nesting_level, jump_size))
            {
                EmitBranch(OP_GOTO, method_stack -> BreakLabel(nesting_level),
                           over);
            }
            return true;
        }
    case Ast::CONTINUE: // JLS 14.14
        {
            unsigned nesting_level =
                statement -> ContinueStatementCast() -> nesting_level;
            AstBlock* over = method_stack -> Block(nesting_level);
            u2 jump_size = (over -> RightToken() - over -> LeftToken() <
                            TOKEN_WIDTH_REQUIRING_GOTOW) ? 3 : 5;
            if (ProcessAbruptExit(nesting_level, jump_size))
            {
                EmitBranch(OP_GOTO,
                           method_stack -> ContinueLabel(nesting_level),
                           over);
            }
            return true;
        }
    case Ast::RETURN: // JLS 14.15
        EmitReturnStatement(statement -> ReturnStatementCast());
        return true;
    case Ast::SUPER_CALL:
        EmitSuperInvocation((AstSuperCall*) statement);
        return false;
    case Ast::THIS_CALL:
        EmitThisInvocation((AstThisCall*) statement);
        return false;
    case Ast::THROW: // JLS 14.16
        EmitExpression(statement -> ThrowStatementCast() -> expression);
        PutOp(OP_ATHROW);
        return true;
    case Ast::SYNCHRONIZED_STATEMENT: // JLS 14.17
        return EmitSynchronizedStatement((AstSynchronizedStatement*) statement);
    case Ast::TRY: // JLS 14.18
        EmitTryStatement((AstTryStatement*) statement);
        return ! statement -> can_complete_normally;
    case Ast::CATCH:   // JLS 14.18
    case Ast::FINALLY: // JLS 14.18
        // handled by TryStatement
        assert(false && "should not get here");
        return false;
    case Ast::ASSERT: // JDK 1.4 (JSR 41)
        EmitAssertStatement((AstAssertStatement*) statement);
        return false;
    case Ast::LOCAL_CLASS: // Class Declaration
        //
        // This is factored out by the front end; and so must be
        // skipped here (remember, interfaces cannot be declared locally).
        //
        return false;
    case Ast::VARIABLE_DECLARATOR:
        //
        // This is not really a statement, but we treat it as one to make
        // initializer blocks easier to intermix with variable declarations.
        //
        InitializeVariable((AstVariableDeclarator*) statement);
        return false;
    default:
        assert(false && "unknown statement kind");
        return false;
    }
}


void ByteCode::EmitReturnStatement(AstReturnStatement* statement)
{
    AstExpression* expression = statement -> expression_opt;

    if (! expression)
    {
        if (ProcessAbruptExit(method_stack -> NestingLevel(0), 1))
            PutOp(OP_RETURN);
    }
    else
    {
        TypeSymbol* type = expression -> Type();
        assert(type != control.void_type);

        EmitExpression(expression);

        if (ProcessAbruptExit(method_stack -> NestingLevel(0), 1, type))
            GenerateReturn(type);
    }
}


bool ByteCode::EmitBlockStatement(AstBlock* block)
{
    assert(stack_depth == 0); // stack empty at start of statement

    method_stack -> Push(block);
    bool abrupt = false;
    for (unsigned i = 0; i < block -> NumStatements() && ! abrupt; i++)
        abrupt = EmitStatement(block -> Statement(i));

    //
    // If contained break statements jump out of this block, define the label.
    //
    if (IsLabelUsed(method_stack -> TopBreakLabel()))
    {
        DefineLabel(method_stack -> TopBreakLabel());
        CompleteLabel(method_stack -> TopBreakLabel());
        abrupt = false;
    }

    if (control.option.g & JopaOption::VARS)
    {
        for (unsigned i = 0; i < block -> NumLocallyDefinedVariables(); i++)
        {
            VariableSymbol* variable = block -> LocallyDefinedVariable(i);
            if (method_stack -> StartPc(variable) == 0xFFFF) // never used
                continue;
#ifdef DUMP
            Coutput << "(56) The symbol \"" << variable -> Name()
                    << "\" numbered " << variable -> LocalVariableIndex()
                    << " was released" << endl;
#endif // DUMP
            local_variable_table_attribute ->
                AddLocalVariable(method_stack -> StartPc(variable),
                                 code_attribute -> CodeLength(),
                                 RegisterName(variable -> ExternalIdentity()),
                                 RegisterUtf8(variable -> Type() -> signature),
                                 variable -> LocalVariableIndex());
        }
    }

    method_stack -> Pop();
    return abrupt;
}


void ByteCode::EmitStatementExpression(AstExpression* expression)
{
    switch (expression -> kind)
    {
    case Ast::CALL:
        EmitMethodInvocation((AstMethodInvocation*) expression, false);
        break;
    case Ast::POST_UNARY:
        EmitPostUnaryExpression((AstPostUnaryExpression*) expression, false);
        break;
    case Ast::PRE_UNARY:
        EmitPreUnaryExpression((AstPreUnaryExpression*) expression, false);
        break;
    case Ast::ASSIGNMENT:
        EmitAssignmentExpression((AstAssignmentExpression*) expression, false);
        break;
    case Ast::CLASS_CREATION:
        EmitClassCreationExpression((AstClassCreationExpression*) expression,
                                    false);
        break;
    default:
        assert(false && "invalid statement expression kind");
    }
}


//
// Generate code for switch statement. Good code generation requires
// detailed knowledge of the target machine. Lacking this, we simply
// choose between LOOKUPSWITCH and TABLESWITCH by picking that
// opcode that takes the least number of bytes in the byte code.
//
// With TABLESWITCH, a target must be provided for every entry in the range
// low..high, even though the user may not have provided an explicit entry,
// in which case the default action is to be taken. For example
// switch (e) {
//  case 1:2:3: act1; break;
//  case 5:6:   act2; break;
//  default: defact; break;
// }
// translates as
// switch (e) {
//  case 1:2:3: act1; break;
//  case 4: goto defa:
//  case 5:6:   act2; break;
//  defa:
//  default: defact;
// }
//
bool ByteCode::EmitSwitchStatement(AstSwitchStatement* switch_statement)
{
    AstBlock* switch_block = switch_statement -> switch_block;
    u2 op_start = code_attribute -> CodeLength();
    unsigned i;
    bool abrupt;

    assert(stack_depth == 0); // stack empty at start of statement

    //
    // String switches require special handling
    //
    if (switch_statement -> is_string_switch)
    {
        return EmitStringSwitchStatement(switch_statement);
    }

    //
    // Optimization: When switching on a constant, emit only those blocks
    // that it will flow through.
    // switch (constant) { ... } => single code path
    //
    if (switch_statement -> expression -> IsConstant())
    {
        CaseElement* target = switch_statement ->
            CaseForValue(DYNAMIC_CAST<IntLiteralValue*>
                         (switch_statement -> expression -> value) -> value);
        if (! target)
            return false;
        //
        // Bring all previously-declared variables into scope, then compile
        // until we run out of blocks or else complete abruptly.
        //
        method_stack -> Push(switch_block);
        for (i = 0; i < target -> block_index; i++)
            EmitSwitchBlockStatement(switch_statement -> Block(i), true);
        abrupt = false;
        for ( ; ! abrupt && i < switch_statement -> NumBlocks(); i++)
        {
            abrupt =
                EmitSwitchBlockStatement(switch_statement -> Block(i), abrupt);
        }

        CloseSwitchLocalVariables(switch_block, op_start);
        if (IsLabelUsed(method_stack -> TopBreakLabel()))
        {
            abrupt = false;
            DefineLabel(method_stack -> TopBreakLabel());
            CompleteLabel(method_stack -> TopBreakLabel());
        }
        method_stack -> Pop();
        return abrupt;
    }

    //
    // Optimization: When there are zero blocks, emit the expression.
    // switch (expr) {} => expr;
    //
    if (! switch_statement -> NumBlocks())
    {
        EmitExpression(switch_statement -> expression, false);
        return false;
    }

    //
    // Optimization: When there is one block labeled by default, emit it.
    // switch (expr) { default: block; } => expr, block
    // switch (expr) { case a: default: block; } => expr, block
    //
    if (switch_statement -> NumBlocks() == 1 &&
        switch_statement -> DefaultCase())
    {
        EmitExpression(switch_statement -> expression, false);
        method_stack -> Push(switch_block);
        abrupt = EmitSwitchBlockStatement(switch_statement -> Block(0), false);
        CloseSwitchLocalVariables(switch_block, op_start);
        if (IsLabelUsed(method_stack -> TopBreakLabel()))
        {
            abrupt = false;
            DefineLabel(method_stack -> TopBreakLabel());
            CompleteLabel(method_stack -> TopBreakLabel());
        }
        method_stack -> Pop();
        return abrupt;
    }

    //
    // Optimization: If there is one non-default label, turn this into an
    // if statement.
    //
    if (switch_statement -> NumCases() == 1)
    {
        //
        // switch (expr) { case a: block; } => if (expr == a) block;
        //
        if (! switch_statement -> DefaultCase())
        {
            EmitExpression(switch_statement -> expression);
            Label lab;
            if (switch_statement -> Case(0) -> value)
            {
                LoadImmediateInteger(switch_statement -> Case(0) -> value);
                EmitBranch(OP_IF_ICMPNE, lab, switch_block);
            }
            else EmitBranch(OP_IFNE, lab, switch_block);
            method_stack -> Push(switch_block);
            EmitSwitchBlockStatement(switch_statement -> Block(0), false);
            CloseSwitchLocalVariables(switch_block, op_start);
            if (IsLabelUsed(method_stack -> TopBreakLabel()))
            {
                DefineLabel(method_stack -> TopBreakLabel());
                CompleteLabel(method_stack -> TopBreakLabel());
            }
            method_stack -> Pop();
            DefineLabel(lab);
            CompleteLabel(lab);
            return false;
        }
        //
        // TODO: Implement these optimizations.
        // switch (expr) { case a: fallthrough_block; default: block; }
        //  => if (expr == a) fallthrough_block; block;
        // switch (expr) { case a: abrupt_block; default: block; }
        //  => if (expr == a) abrupt_block; else block;
        // switch (expr) { default: fallthrough_block; case a: block; }
        //  => if (expr != a) fallthrough_block; block;
        // switch (expr) { default: abrupt_block; case a: block; }
        //  => if (expr != a) abrupt_block; else block;
        //
    }

    //
    // Use tableswitch if size of tableswitch case is no more than 32 bytes
    // (8 words) more code than lookup case.
    //
    bool use_lookup = true; // set if using LOOKUPSWITCH opcode
    unsigned ncases = switch_statement -> NumCases();
    unsigned nlabels = ncases;
    i4 high = 0,
       low = 0;
    if (ncases)
    {
        low = switch_statement -> Case(0) -> value;
        high = switch_statement -> Case(ncases - 1) -> value;
        assert(low <= high);

        //
        // Workaround for Sun JVM TABLESWITCH bug in JDK 1.2, 1.3
        // when case values of 0x7ffffff0 through 0x7fffffff are used.
        // Force the generation of a LOOKUPSWITCH in these circumstances.
        //
        if (high < 0x7ffffff0L ||
            control.option.target >= JopaOption::SDK1_4)
        {
            // We want to compute (1 + (high - low + 1)) < (ncases * 2 + 8).
            // However, we must beware of integer overflow.
            i4 range = high - low + 1;
            if (range > 0 && (unsigned) range < (ncases * 2 + 8))
            {
                use_lookup = false; // use tableswitch
                nlabels = range;
                assert(nlabels >= ncases);
            }
        }
    }

    //
    // Set up the environment for the switch block.  This must be done before
    // emitting the expression, in case the expression is an assignment.
    //
    method_stack -> Push(switch_block);

    //
    // Reset the line number before evaluating the expression
    //
    line_number_table_attribute ->
        AddLineNumber(code_attribute -> CodeLength(),
                      semantic.lex_stream -> Line(switch_statement ->
                                                  expression -> LeftToken()));
    EmitExpression(switch_statement -> expression);

    PutOp(use_lookup ? OP_LOOKUPSWITCH : OP_TABLESWITCH);
    op_start = last_op_pc; // pc at start of instruction

    //
    // Supply any needed padding.
    //
    while (code_attribute -> CodeLength() % 4 != 0)
        PutU1(0);

    //
    // Note that if there is no default clause in switch statement, we create
    // one that corresponds to do nothing and branches to start of next
    // statement. The default label is case_labels[nlabels].
    //
    Label* case_labels = new Label[nlabels + 1];
    UseLabel(case_labels[nlabels], 4,
             code_attribute -> CodeLength() - op_start);

    if (use_lookup)
    {
        PutU4(ncases);
        for (i = 0; i < ncases; i++)
        {
            PutU4(switch_statement -> Case(i) -> value);
            UseLabel(case_labels[i], 4,
                     code_attribute -> CodeLength() - op_start);
        }
    }
    else
    {
        PutU4(low);
        PutU4(high);
        for (i = 0; i < nlabels; i++)
        {
            UseLabel(case_labels[i], 4,
                     code_attribute -> CodeLength() - op_start);
        }
    }

    //
    // March through switch block statements, compiling blocks in proper
    // order. We must respect order in which blocks are seen so that blocks
    // lacking a terminal break fall through to the proper place.
    //
    abrupt = false;
    for (i = 0; i < switch_block -> NumStatements(); i++)
    {
        AstSwitchBlockStatement* switch_block_statement =
            switch_statement -> Block(i);
        for (unsigned li = 0;
             li < switch_block_statement -> NumSwitchLabels(); li++)
        {
            AstSwitchLabel* switch_label =
                switch_block_statement -> SwitchLabel(li);
            if (use_lookup)
                DefineLabel(case_labels[switch_label -> map_index]);
            else if (switch_label -> expression_opt)
            {
                i4 value = DYNAMIC_CAST<IntLiteralValue*>
                    (switch_label -> expression_opt -> value) -> value;
                DefineLabel(case_labels[value - low]);
            }
            else
            {
                DefineLabel(case_labels[nlabels]);
                //
                // We must also point all inserted cases to the default.
                //
                unsigned j = 1;
                i4 k = low + 1;
                for ( ; j < switch_statement -> NumCases(); j++, k++)
                    while (k != switch_statement -> Case(j) -> value)
                        DefineLabel(case_labels[k++ - low]);
            }
        }
        abrupt = EmitSwitchBlockStatement(switch_block_statement, false);
    }

    CloseSwitchLocalVariables(switch_block, op_start);
    for (i = 0; i <= nlabels; i++)
    {
        if (! case_labels[i].defined)
        {
            abrupt = false;
            DefineLabel(case_labels[i]);
        }
        CompleteLabel(case_labels[i]);
    }
    //
    // If this switch statement was "broken", we define the break label here.
    //
    if (IsLabelUsed(method_stack -> TopBreakLabel()))
    {
        // need define only if used
        DefineLabel(method_stack -> TopBreakLabel());
        CompleteLabel(method_stack -> TopBreakLabel());
        abrupt = false;
    }

    delete [] case_labels;
    method_stack -> Pop();
    assert(abrupt || switch_statement -> can_complete_normally);
    return abrupt;
}


bool ByteCode::EmitStringSwitchStatement(AstSwitchStatement* switch_statement)
{
    AstBlock* switch_block = switch_statement -> switch_block;
    u2 op_start = code_attribute -> CodeLength();
    unsigned i;
    bool abrupt;

    assert(stack_depth == 0);

    if (! switch_statement -> NumBlocks())
    {
        EmitExpression(switch_statement -> expression, false);
        return false;
    }

    method_stack -> Push(switch_block);

    line_number_table_attribute ->
        AddLineNumber(code_attribute -> CodeLength(),
                      semantic.lex_stream -> Line(switch_statement ->
                                                  expression -> LeftToken()));

    EmitExpression(switch_statement -> expression);

    u2 string_var = switch_block -> block_symbol -> helper_variable_index;

    if (string_var <= 3)
        PutOp((Opcode) (OP_ASTORE_0 + string_var));
    else
        PutOpWide(OP_ASTORE, string_var);

    Label default_jump_label;

    if (string_var <= 3)
        PutOp((Opcode) (OP_ALOAD_0 + string_var));
    else
        PutOpWide(OP_ALOAD, string_var);
    EmitBranch(OP_IFNULL, default_jump_label, switch_block);

    if (string_var <= 3)
        PutOp((Opcode) (OP_ALOAD_0 + string_var));
    else
        PutOpWide(OP_ALOAD, string_var);
    PutOp(OP_INVOKEVIRTUAL);
    PutU2(RegisterLibraryMethodref(control.String_hashCodeMethod()));
    ChangeStack(1);

    unsigned ncases = switch_statement -> NumCases();
    unsigned nlabels = ncases;
    i4 high = 0, low = 0;
    bool use_lookup = true;

    if (ncases)
    {
        low = switch_statement -> Case(0) -> value;
        high = switch_statement -> Case(ncases - 1) -> value;
        i4 range = high - low + 1;
        if (range > 0 && (unsigned) range < (ncases * 2 + 8))
        {
            use_lookup = false;
            nlabels = range;
        }
    }

    PutOp(use_lookup ? OP_LOOKUPSWITCH : OP_TABLESWITCH);
    u2 switch_op_start = last_op_pc;

    while (code_attribute -> CodeLength() % 4 != 0)
        PutU1(0);

    Label* hash_labels = new Label[nlabels + 1];
    UseLabel(hash_labels[nlabels], 4,
             code_attribute -> CodeLength() - switch_op_start);

    if (use_lookup)
    {
        PutU4(ncases);
        for (i = 0; i < ncases; i++)
        {
            PutU4(switch_statement -> Case(i) -> value);
            UseLabel(hash_labels[i], 4,
                     code_attribute -> CodeLength() - switch_op_start);
        }
    }
    else
    {
        PutU4(low);
        PutU4(high);
        unsigned j = 0;
        for (i4 k = low; k <= high; k++, j++)
        {
            while (j < ncases && switch_statement -> Case(j) -> value < k)
                j++;
            if (j < ncases && switch_statement -> Case(j) -> value == k)
            {
                UseLabel(hash_labels[j], 4,
                         code_attribute -> CodeLength() - switch_op_start);
            }
            else
            {
                UseLabel(hash_labels[nlabels], 4,
                         code_attribute -> CodeLength() - switch_op_start);
            }
        }
    }

    Label* case_labels = new Label[switch_statement -> NumCases() + 1];

    for (i = 0; i < ncases; i++)
    {
        DefineLabel(hash_labels[i]);
        CompleteLabel(hash_labels[i]);

        unsigned j = i;
        while (j < ncases && switch_statement -> Case(j) -> value ==
               switch_statement -> Case(i) -> value)
        {
            CaseElement* case_elt = switch_statement -> Case(j);
            AstSwitchLabel* label = switch_statement ->
                Block(case_elt -> block_index) ->
                SwitchLabel(case_elt -> case_index);

            if (string_var <= 3)
                PutOp((Opcode) (OP_ALOAD_0 + string_var));
            else
                PutOpWide(OP_ALOAD, string_var);

            Utf8LiteralValue* str_value = DYNAMIC_CAST<Utf8LiteralValue*>
                (label -> expression_opt -> value);
            PutOp(OP_LDC_W);
            PutU2(RegisterString(str_value));
            // Note: LDC_W already adds +1 to stack via PutOp

            PutOp(OP_INVOKEVIRTUAL);
            PutU2(RegisterLibraryMethodref(control.String_equalsMethod()));
            // equals(Object)Z: default -1 for receiver, 0 adjustment needed
            // (arg -1 + return +1 = 0)

            EmitBranch(OP_IFNE, case_labels[j], switch_block);
            j++;
        }
        i = j - 1;
        EmitBranch(OP_GOTO, hash_labels[nlabels], switch_block);
    }

    // Emit case bodies in order. Define labels at the appropriate positions.
    abrupt = false;
    for (unsigned block_idx = 0; block_idx < switch_block -> NumStatements();
         block_idx++)
    {
        AstSwitchBlockStatement* switch_block_stmt =
            switch_statement -> Block(block_idx);

        for (unsigned label_idx = 0;
             label_idx < switch_block_stmt -> NumSwitchLabels(); label_idx++)
        {
            AstSwitchLabel* switch_label =
                switch_block_stmt -> SwitchLabel(label_idx);
            if (switch_label -> expression_opt)
            {
                // Non-default case: define case label
                DefineLabel(case_labels[switch_label -> map_index]);
                CompleteLabel(case_labels[switch_label -> map_index]);
            }
            else
            {
                // Default case: define hash no-match and null-check labels here
                DefineLabel(hash_labels[nlabels]);
                CompleteLabel(hash_labels[nlabels]);
                DefineLabel(default_jump_label);
                CompleteLabel(default_jump_label);
            }
        }

        abrupt = EmitSwitchBlockStatement(switch_block_stmt, false);
    }

    CloseSwitchLocalVariables(switch_block, op_start);

    // Complete any undefined case labels (and no-match/default if no default case)
    for (i = 0; i < ncases; i++)
    {
        if (! case_labels[i].defined)
        {
            abrupt = false;
            DefineLabel(case_labels[i]);
        }
        CompleteLabel(case_labels[i]);
    }

    // If no default case was defined, define the no-match and null labels here
    if (! hash_labels[nlabels].defined)
    {
        DefineLabel(hash_labels[nlabels]);
        abrupt = false;
    }
    CompleteLabel(hash_labels[nlabels]);

    if (! default_jump_label.defined)
    {
        DefineLabel(default_jump_label);
        abrupt = false;
    }
    CompleteLabel(default_jump_label);

    if (IsLabelUsed(method_stack -> TopBreakLabel()))
    {
        DefineLabel(method_stack -> TopBreakLabel());
        CompleteLabel(method_stack -> TopBreakLabel());
        abrupt = false;
    }

    delete [] hash_labels;
    delete [] case_labels;
    method_stack -> Pop();
    return abrupt;
}


bool ByteCode::EmitSwitchBlockStatement(AstSwitchBlockStatement* block,
                                        bool abrupt)
{
    for (unsigned i = 0; i < block -> NumStatements(); i++)
    {
        if (! abrupt)
            abrupt = EmitStatement(block -> Statement(i));
        else if (block -> Statement(i) -> LocalVariableStatementCast())
        {
            //
            // In a switch statement, local variable declarations are
            // accessible in other case labels even if the declaration
            // itself is unreachable.
            //
            AstLocalVariableStatement* lvs =
                (AstLocalVariableStatement*) block -> Statement(i);
            for (unsigned j = 0; j < lvs -> NumVariableDeclarators(); j++)
            {
                AstVariableDeclarator* declarator =
                    lvs -> VariableDeclarator(j);
                if (control.option.g & JopaOption::VARS)
                {
                    method_stack -> StartPc(declarator -> symbol) =
                        code_attribute -> CodeLength();
                }
                if (declarator -> symbol -> Type() -> num_dimensions > 255)
                {
                    semantic.ReportSemError(SemanticError::ARRAY_OVERFLOW,
                                            declarator);
                }
            }
        }
    }
    return abrupt;
}


void ByteCode::CloseSwitchLocalVariables(AstBlock* switch_block,
                                         u2 op_start)
{
    if (control.option.g & JopaOption::VARS)
    {
        for (unsigned i = 0;
             i < switch_block -> NumLocallyDefinedVariables(); i++)
        {
            VariableSymbol* variable =
                switch_block -> LocallyDefinedVariable(i);
            if (method_stack -> StartPc(variable) > op_start)
            {
                if (method_stack -> StartPc(variable) == 0xFFFF) // never used
                    continue;
#ifdef DUMP
                Coutput << "(58) The symbol \"" << variable -> Name()
                        << "\" numbered " << variable -> LocalVariableIndex()
                        << " was released" << endl;
#endif // DUMP
                local_variable_table_attribute ->
                    AddLocalVariable(method_stack -> StartPc(variable),
                                     code_attribute -> CodeLength(),
                                     RegisterName(variable -> ExternalIdentity()),
                                     RegisterUtf8(variable -> Type() -> signature),
                                     variable -> LocalVariableIndex());
            }
        }
    }
}


//
// Java 7: Helper to emit close() calls for try-with-resources
// Closes resources in reverse declaration order with exception suppression.
//
// Exception handling follows JLS 14.20.3:
// - If primary_exception exists (try body threw): catch close exceptions and
//   call primary_exception.addSuppressed(closeException)
// - If primary_exception is null: accumulate first close exception and
//   suppress subsequent ones with close_exception.addSuppressed()
// - After all closes, throw close_exception if primary_exception is null
//
// Variable layout (relative to variable_index):
//   +0: primary_exception (set by catch-all handler if try throws)
//   +2: close_exception (accumulated close exceptions in normal path)
//
void ByteCode::EmitResourceCleanup(AstTryStatement* statement, int variable_index)
{
    // Close resources in reverse order
    for (int r = statement -> NumResources() - 1; r >= 0; r--)
    {
        AstLocalVariableStatement* resource = statement -> Resource(r);
        for (int v = resource -> NumVariableDeclarators() - 1; v >= 0; v--)
        {
            AstVariableDeclarator* vd = resource -> VariableDeclarator(v);
            VariableSymbol* var = vd -> symbol;
            if (! var) continue;

            // Load the resource variable
            LoadLocal(var -> LocalVariableIndex(), var -> Type());

            // Check for null before calling close()
            Label skip_close;
            Label after_catch;
            PutOp(OP_DUP);
            int stack_at_branch = stack_depth;  // = 2

            EmitBranch(OP_IFNULL, skip_close);
            // Fall-through (not null) path: stack = 1 [ref]

            // Record start of try block for close()
            u2 close_try_start = code_attribute -> CodeLength();

            // Call close() - AutoCloseable is an interface
            PutOp(OP_INVOKEINTERFACE);
            PutU2(RegisterLibraryMethodref(control.AutoCloseable_closeMethod()));
            PutU1(1);  // 1 argument (just 'this')
            PutU1(0);  // reserved
            // After close(): stack = 0

            // Record end of try block
            u2 close_try_end = code_attribute -> CodeLength();

            EmitBranch(OP_GOTO, after_catch, statement);

            // Exception handler for close(): catches any Throwable
            u2 catch_handler_pc = code_attribute -> CodeLength();
            stack_depth = 1;  // caught exception is on stack

            // Logic:
            // if (primary_exception != null):
            //     primary_exception.addSuppressed(caught)
            // else if (close_exception != null):
            //     close_exception.addSuppressed(caught)
            // else:
            //     close_exception = caught

            Label check_close_exception;
            Label first_close_exception;

            // Load primary_exception and check if non-null
            // Stack: [caught]
            LoadLocal(variable_index, control.Throwable());
            // Stack: [caught, primary]
            PutOp(OP_DUP);
            // Stack: [caught, primary, primary]
            EmitBranch(OP_IFNULL, check_close_exception);
            // Stack: [caught, primary]

            // Primary exception exists: call primary.addSuppressed(caught)
            PutOp(OP_SWAP);
            // Stack: [primary, caught]
            PutOp(OP_INVOKEVIRTUAL);
            PutU2(RegisterLibraryMethodref(control.Throwable_addSuppressedMethod()));
            // Stack: []
            EmitBranch(OP_GOTO, after_catch, statement);

            // check_close_exception:
            stack_depth = 2;  // [caught, null]
            DefineLabel(check_close_exception);
            CompleteLabel(check_close_exception);
            PutOp(OP_POP);  // pop null (was primary)
            // Stack: [caught]

            // Load close_exception and check if non-null
            LoadLocal(variable_index + 2, control.Throwable());
            // Stack: [caught, close]
            PutOp(OP_DUP);
            // Stack: [caught, close, close]
            EmitBranch(OP_IFNULL, first_close_exception);
            // Stack: [caught, close]

            // Close exception exists: call close.addSuppressed(caught)
            PutOp(OP_SWAP);
            // Stack: [close, caught]
            PutOp(OP_INVOKEVIRTUAL);
            PutU2(RegisterLibraryMethodref(control.Throwable_addSuppressedMethod()));
            // Stack: []
            EmitBranch(OP_GOTO, after_catch, statement);

            // first_close_exception: no previous exception, store caught as close_exception
            stack_depth = 2;  // [caught, null]
            DefineLabel(first_close_exception);
            CompleteLabel(first_close_exception);
            PutOp(OP_POP);  // pop null (was close)
            // Stack: [caught]
            StoreLocal(variable_index + 2, control.Throwable());
            // Stack: []
            EmitBranch(OP_GOTO, after_catch, statement);

            // Null path: resource was null, just pop it
            stack_depth = stack_at_branch - 1;  // IFNULL pops 1
            DefineLabel(skip_close);
            CompleteLabel(skip_close);
            PutOp(OP_POP);  // pop the null reference
            // Stack: []

            // After all paths converge
            stack_depth = 0;
            DefineLabel(after_catch);
            CompleteLabel(after_catch);

            // Register exception handler for close() call
            // Catch type is java/lang/Throwable
            code_attribute -> AddException(close_try_start, close_try_end,
                                           catch_handler_pc,
                                           RegisterClass(control.Throwable()));
        }
    }

    //
    // After all resources are closed, check if we need to throw close_exception.
    // This happens when primary_exception is null (normal path) and close_exception
    // is non-null (some close() threw).
    //
    Label done_cleanup;
    // if (primary_exception != null) goto done_cleanup (it will be rethrown by caller)
    LoadLocal(variable_index, control.Throwable());
    EmitBranch(OP_IFNONNULL, done_cleanup);
    // stack = 0 after IFNONNULL

    // if (close_exception == null) goto done_cleanup
    LoadLocal(variable_index + 2, control.Throwable());
    EmitBranch(OP_IFNULL, done_cleanup);
    // stack = 0 after IFNULL

    // Reload close_exception and throw it
    LoadLocal(variable_index + 2, control.Throwable());
    PutOp(OP_ATHROW);

    // done_cleanup: both paths arrive with stack = 0
    stack_depth = 0;
    DefineLabel(done_cleanup);
    CompleteLabel(done_cleanup);
}

//
//  13.18       The try statement
//
void ByteCode::EmitTryStatement(AstTryStatement* statement)
{
    //
    // If the finally label in the surrounding block is used by a try
    // statement, it is cleared after the finally block associated with the
    // try statement has been processed.
    //
    assert(method_stack -> TopFinallyLabel().uses.Length() == 0);
    assert(method_stack -> TopFinallyLabel().defined == false);
    assert(method_stack -> TopFinallyLabel().definition == 0);

    //
    // Java 7: Emit resource initializations for try-with-resources
    // Resources are stored in local variables before the try block
    //
    bool has_resources = statement -> NumResources() > 0;
    for (unsigned r = 0; r < statement -> NumResources(); r++)
    {
        AstLocalVariableStatement* resource = statement -> Resource(r);
        for (unsigned v = 0; v < resource -> NumVariableDeclarators(); v++)
        {
            AstVariableDeclarator* vd = resource -> VariableDeclarator(v);
            DeclareLocalVariable(vd);
        }
    }

    //
    // Java 7: Initialize exception variables for try-with-resources suppression.
    // variable_index+0: primary exception (set by catch-all handler if try throws)
    // variable_index+2: close exception (accumulated close exceptions in normal path)
    //
    if (has_resources)
    {
        int variable_index = method_stack -> TopBlock() ->
            block_symbol -> helper_variable_index;
        PutOp(OP_ACONST_NULL);
        StoreLocal(variable_index, control.Throwable());  // primary_exception = null
        PutOp(OP_ACONST_NULL);
        StoreLocal(variable_index + 2, control.Throwable());  // close_exception = null
    }

    u2 start_try_block_pc = code_attribute -> CodeLength(); // start pc
    assert(method_stack -> TopHandlerRangeStart().Length() == 0 &&
           method_stack -> TopHandlerRangeEnd().Length() == 0);
    method_stack -> TopHandlerRangeStart().Push(start_try_block_pc);

    // Java 7: Try-with-resources creates a synthetic finally for close() calls
    bool emit_explicit_finally = statement -> finally_clause_opt &&
        ! IsNop(statement -> finally_clause_opt -> block);
    bool emit_finally_clause = emit_explicit_finally || has_resources;

    //
    // If we determined the finally clause is a nop, remove the tag
    // TRY_CLAUSE_WITH_FINALLY so that abrupt completions do not emit JSR.
    // On the other hand, if the finally clause cannot complete normally,
    // change the tag to ABRUPT_TRY_FINALLY so that abrupt completions emit
    // a GOTO instead of a JSR. Also, mark a try block which has a catch
    // clause but no finally clause, in case an abrupt exit forces a split
    // in the range of protected code.
    //
    if (statement -> finally_clause_opt)
    {
        if (! emit_explicit_finally && ! has_resources)
        {
            statement -> block -> SetTag(AstBlock::NONE);
        }
        else if (! statement -> finally_clause_opt -> block ->
                 can_complete_normally)
        {
            statement -> block -> SetTag(AstBlock::ABRUPT_TRY_FINALLY);
        }
    }
    // For try-with-resources without explicit finally, set the finally tag
    else if (has_resources)
    {
        statement -> block -> SetTag(AstBlock::TRY_CLAUSE_WITH_FINALLY);
    }
    if (statement -> block -> Tag() == AstBlock::NONE &&
        statement -> NumCatchClauses())
    {
        statement -> block -> SetTag(AstBlock::TRY_CLAUSE_WITH_CATCH);
    }
    bool abrupt = EmitBlockStatement(statement -> block);

    //
    // The computation of end_try_block_pc, the instruction following the last
    // instruction in the body of the try block, does not include the code, if
    // any, needed to call a finally block or skip to the end of the try
    // statement.
    //
    u2 end_try_block_pc = code_attribute -> CodeLength();
    Tuple<u2> handler_starts(method_stack -> TopHandlerRangeStart());
    Tuple<u2> handler_ends(method_stack -> TopHandlerRangeEnd());
    handler_ends.Push(end_try_block_pc);
    assert(handler_starts.Length() == handler_ends.Length());

    //
    // If try block is not empty, process catch clauses, including "special"
    // clause for finally.
    //
    if (start_try_block_pc != end_try_block_pc)
    {
        // Use the label in the block immediately enclosing try statement.
        Label& finally_label = method_stack -> TopFinallyLabel();
        Label end_label;

        //
        // If try block completes normally, skip code for catch blocks.
        //
        if (! abrupt &&
            (emit_finally_clause || statement -> NumCatchClauses()))
        {
            EmitBranch(OP_GOTO, end_label, statement);
        }

        for (unsigned i = 0; i < statement -> NumCatchClauses(); i++)
        {
            u2 handler_pc = code_attribute -> CodeLength();

            AstCatchClause* catch_clause = statement -> CatchClause(i);
            VariableSymbol* parameter_symbol =
                catch_clause -> parameter_symbol;

            assert(stack_depth == 0);
            stack_depth = 1; // account for the exception already on the stack
            line_number_table_attribute ->
                AddLineNumber(code_attribute -> CodeLength(),
                              semantic.lex_stream -> Line(catch_clause ->
                                                          catch_token));
            //
            // Unless debugging, we don't need to waste a variable on an
            // empty catch.
            //
            if ((control.option.g & JopaOption::VARS) ||
                ! IsNop(catch_clause -> block))
            {
                StoreLocal(parameter_symbol -> LocalVariableIndex(),
                           parameter_symbol -> Type());
            }
            else PutOp(OP_POP);
            u2 handler_type = RegisterClass(parameter_symbol -> Type());
            for (int j = handler_starts.Length(); --j >= 0; )
            {
                code_attribute ->
                    AddException(handler_starts[j], handler_ends[j],
                                 handler_pc, handler_type);
            }

            //
            // If we determined the finally clause is a nop, remove the tag
            // TRY_CLAUSE_WITH_FINALLY so that abrupt completions do not emit
            // JSR. On the other hand, if the finally clause cannot complete
            // normally, change the tag to ABRUPT_TRY_FINALLY so that abrupt
            // completions emit a GOTO instead of a JSR.
            //
            if (statement -> finally_clause_opt)
            {
                if (! emit_finally_clause)
                    catch_clause -> block -> SetTag(AstBlock::NONE);
                else if (! statement -> finally_clause_opt -> block ->
                         can_complete_normally)
                {
                    catch_clause -> block ->
                        SetTag(AstBlock::ABRUPT_TRY_FINALLY);
                }
            }
            abrupt = EmitBlockStatement(catch_clause -> block);

            if (control.option.g & JopaOption::VARS)
            {
                local_variable_table_attribute ->
                    AddLocalVariable(handler_pc,
                                     code_attribute -> CodeLength(),
                                     RegisterName(parameter_symbol -> ExternalIdentity()),
                                     RegisterUtf8(parameter_symbol -> Type() -> signature),
                                     parameter_symbol -> LocalVariableIndex());
            }

            //
            // If catch block completes normally, skip further catch blocks.
            //
            if (! abrupt && (emit_finally_clause ||
                             i < (statement -> NumCatchClauses() - 1)))
            {
                EmitBranch(OP_GOTO, end_label, statement);
            }
        }
        //
        // If this try statement contains a finally clause (explicit or synthetic), then ...
        //
        if (emit_finally_clause)
        {
            int variable_index = method_stack -> TopBlock() ->
                block_symbol -> helper_variable_index;
            u2 finally_start_pc = code_attribute -> CodeLength();
            u2 special_end_pc = finally_start_pc;

            // For try-with-resources, the synthetic finally can always complete normally
            // (it just calls close() on resources)
            bool finally_can_complete = has_resources ||
                (emit_explicit_finally && statement -> finally_clause_opt -> block ->
                 can_complete_normally);

            //
            // Emit code for "special" handler to make sure finally clause is
            // invoked in case an otherwise uncaught exception is thrown in the
            // try block, or an exception is thrown from within a catch block.
            // This must cover all instructions through the jsr, in case of
            // asynchronous exceptions.
            //
            assert(stack_depth == 0);
            stack_depth = 1; // account for the exception already on stack
            if (finally_can_complete)
            {
                StoreLocal(variable_index, control.Throwable()); // Save,
                EmitBranch(OP_JSR, finally_label, statement);
                special_end_pc = code_attribute -> CodeLength();
                LoadLocal(variable_index, control.Throwable()); // reload, and
                PutOp(OP_ATHROW); // rethrow exception.
            }
            else
            {
                //
                // Ignore the exception already on the stack, since we know
                // the finally clause overrides it.
                //
                PutOp(OP_POP);
            }
            method_stack -> TopHandlerRangeEnd().Push(special_end_pc);
            unsigned count = method_stack -> TopHandlerRangeStart().Length();
            assert(count == method_stack -> TopHandlerRangeEnd().Length());
            while (count--)
            {
                code_attribute ->
                    AddException(method_stack -> TopHandlerRangeStart().Pop(),
                                 method_stack -> TopHandlerRangeEnd().Pop(),
                                 finally_start_pc, 0);
            }

            //
            // Generate code for finally clause. If the finally block can
            // complete normally, this is reached from a JSR, so save the
            // return address. Otherwise, this is reached from a GOTO.
            //
            DefineLabel(finally_label);
            assert(stack_depth == 0);
            if (finally_can_complete)
            {
                stack_depth = 1; // account for the return location on stack
                StoreLocal(variable_index + 1, control.Object());
            }
            else if (IsLabelUsed(end_label))
            {
                DefineLabel(end_label);
                CompleteLabel(end_label);
            }

            // Java 7: Emit close() calls for resources before explicit finally
            if (has_resources)
            {
                EmitResourceCleanup(statement, variable_index);
            }

            // Emit explicit finally block if present
            if (emit_explicit_finally)
            {
                EmitBlockStatement(statement -> finally_clause_opt -> block);
            }

            //
            // If a finally block can complete normally, return to the saved
            // address of the caller.
            //
            if (finally_can_complete)
            {
                PutOpWide(OP_RET, variable_index + 1);
                //
                // Now, if the try or catch blocks complete normally, execute
                // the finally block before advancing to next statement. We
                // need to trap one more possibility of an asynchronous
                // exception before the jsr has started.
                //
                if (IsLabelUsed(end_label))
                {
                    DefineLabel(end_label);
                    CompleteLabel(end_label);
                    EmitBranch(OP_JSR, finally_label, statement);
                    special_end_pc = code_attribute -> CodeLength();
                    code_attribute -> AddException(special_end_pc - 3,
                                                   special_end_pc,
                                                   finally_start_pc, 0);
                }
            }
            CompleteLabel(finally_label);
        }
        else
        {
            //
            // Finally block is not present, advance to next statement, and
            // clean up the handler start/end ranges.
            //
            assert(! IsLabelUsed(finally_label));
            DefineLabel(end_label);
            CompleteLabel(end_label);
            method_stack -> TopHandlerRangeStart().Reset();
            method_stack -> TopHandlerRangeEnd().Reset();
        }
    }
    else
    {
        //
        // Try block was empty; skip all catch blocks, and a finally block
        // is treated normally. For try-with-resources, just close resources.
        //
        method_stack -> TopHandlerRangeStart().Reset();
        if (has_resources)
        {
            int variable_index = method_stack -> TopBlock() ->
                block_symbol -> helper_variable_index;
            EmitResourceCleanup(statement, variable_index);
        }
        if (emit_explicit_finally)
            EmitBlockStatement(statement -> finally_clause_opt -> block);
    }
}


//
// Exit to block at level, freeing monitor locks and invoking finally
// clauses as appropriate. The width is 1 for return, 3 for normal a normal
// GOTO (from a break or continue), or 5 for a GOTO_W. The return is true
// unless some intervening finally block cannot complete normally.
//
bool ByteCode::ProcessAbruptExit(unsigned level, u2 width,
                                 TypeSymbol* return_type)
{
    int variable_index = -1;
    //
    // We must store the return value in a variable, rather than on the
    // stack, in case a finally block contains an embedded try-catch which
    // wipes out the stack.
    //
    if (return_type)
    {
        for (unsigned i = method_stack -> Size() - 1;
             i > 0 && method_stack -> NestingLevel(i) != level; i--)
        {
            unsigned nesting_level = method_stack -> NestingLevel(i);
            unsigned enclosing_level = method_stack -> NestingLevel(i - 1);
            AstBlock* block = method_stack -> Block(nesting_level);
            if (block -> Tag() == AstBlock::TRY_CLAUSE_WITH_FINALLY)
            {
                variable_index = method_stack -> Block(enclosing_level) ->
                    block_symbol -> helper_variable_index + 2;
            }
            else if (block -> Tag() == AstBlock::ABRUPT_TRY_FINALLY)
            {
                variable_index = -1;
                PutOp(control.IsDoubleWordType(return_type) ? OP_POP2 : OP_POP);
                break;
            }
        }
    }
    if (variable_index >= 0)
        StoreLocal(variable_index, return_type);

    for (unsigned i = method_stack -> Size() - 1;
         i > 0 && method_stack -> NestingLevel(i) != level; i--)
    {
        unsigned nesting_level = method_stack -> NestingLevel(i);
        unsigned enclosing_level = method_stack -> NestingLevel(i - 1);
        AstBlock* block = method_stack -> Block(nesting_level);
        if (block -> Tag() == AstBlock::TRY_CLAUSE_WITH_FINALLY)
        {
            EmitBranch(OP_JSR, method_stack -> FinallyLabel(enclosing_level),
                       method_stack -> Block(enclosing_level));
            method_stack -> HandlerRangeEnd(enclosing_level).
                Push(code_attribute -> CodeLength());
        }
        else if (block -> Tag() == AstBlock::ABRUPT_TRY_FINALLY)
        {
            //
            // Ignore the width of the abrupt instruction, because the abrupt
            // finally preempts it.
            //
            width = 0;
            EmitBranch(OP_GOTO, method_stack -> FinallyLabel(enclosing_level),
                       method_stack -> Block(enclosing_level));
            method_stack -> HandlerRangeEnd(enclosing_level).
                Push(code_attribute -> CodeLength());
            break;
        }
        else if (block -> Tag() == AstBlock::SYNCHRONIZED)
        {
            //
            // This code must be safe for asynchronous exceptions.  Note that
            // we are splitting the range of instructions covered by the
            // synchronized statement catchall handler.
            //
            int variable_index = method_stack -> Block(enclosing_level) ->
                block_symbol -> helper_variable_index;
            LoadLocal(variable_index, control.Object());
            PutOp(OP_MONITOREXIT);
            method_stack -> HandlerRangeEnd(enclosing_level).
                Push(code_attribute -> CodeLength());
        }
        else if (block -> Tag() == AstBlock::TRY_CLAUSE_WITH_CATCH)
        {
            method_stack -> HandlerRangeEnd(enclosing_level).
                Push(code_attribute -> CodeLength());
        }
    }

    if (variable_index >= 0)
        LoadLocal(variable_index, return_type);
    for (unsigned j = method_stack -> Size() - 1;
         j > 0 && method_stack -> NestingLevel(j) != level; j--)
    {
        unsigned nesting_level = method_stack -> NestingLevel(j);
        unsigned enclosing_level = method_stack -> NestingLevel(j - 1);
        AstBlock* block = method_stack -> Block(nesting_level);
        if (block -> Tag() == AstBlock::SYNCHRONIZED ||
            block -> Tag() == AstBlock::TRY_CLAUSE_WITH_CATCH ||
            block -> Tag() == AstBlock::TRY_CLAUSE_WITH_FINALLY)
        {
            method_stack -> HandlerRangeStart(enclosing_level).
                Push(code_attribute -> CodeLength() + width);
        }
        else if (block -> Tag() == AstBlock::ABRUPT_TRY_FINALLY)
        {
            method_stack -> HandlerRangeStart(enclosing_level).
                Push(code_attribute -> CodeLength());
            return false;
        }
    }
    return true;
}

void ByteCode::EmitBranch(Opcode opc, Label& lab, AstStatement* over)
{
    // Use the number of tokens as a heuristic for the size of the statement
    // we're jumping over. If the statement is large enough, either change
    // to the 4-byte branch opcode or write out a branch around a goto_w for
    // branch opcodes that don't have a long form.
    int sizeHeuristic = over ? over -> RightToken() - over -> LeftToken() : 0;
    if (sizeHeuristic < TOKEN_WIDTH_REQUIRING_GOTOW) {
        PutOp(opc);
        UseLabel(lab, 2, 1);
        return;
    }
    if (opc == OP_GOTO) {
        PutOp(OP_GOTO_W);
        UseLabel(lab, 4, 1);
        return;
    }
    if (opc == OP_JSR) {
        PutOp(OP_JSR_W);
        UseLabel(lab, 4, 1);
        return;
    }
    // if op lab
    //  =>
    // if !op label2
    // goto_w lab
    // label2:
    PutOp(InvertIfOpCode(opc));
    Label label2;
    UseLabel(label2, 2, 1);
    PutOp(OP_GOTO_W);
    UseLabel(lab, 4, 1);
    DefineLabel(label2);
    CompleteLabel(label2);
}

//
// java provides a variety of conditional branch instructions, so
// that a number of operators merit special handling:
//      constant operand
//      negation (we eliminate it)
//      equality
//      ?: && and || (partial evaluation)
//      comparisons
// Other expressions are just evaluated and the appropriate
// branch emitted.
//
// TODO: return a bool that is true if the statement being branched over is
// even needed (if statements and other places might have a constant false
// expression, allowing the next block of code to be skipped entirely).
//
void ByteCode::EmitBranchIfExpression(AstExpression* p, bool cond, Label& lab,
                                      AstStatement* over)
{
    p = StripNops(p);
    assert(p -> Type() == control.boolean_type);

    if (p -> IsConstant())
    {
        if (IsZero(p) != cond)
            EmitBranch(OP_GOTO, lab, over);
        return;
    }

    AstPreUnaryExpression* pre = p -> PreUnaryExpressionCast();
    if (pre) // must be !
    {
        //
        // branch_if(!e,c,l) => branch_if(e,!c,l)
        //
        assert(pre -> Tag() == AstPreUnaryExpression::NOT);
        EmitBranchIfExpression(pre -> expression, ! cond, lab, over);
        return;
    }

    AstConditionalExpression* conditional = p -> ConditionalExpressionCast();
    if (conditional)
    {
        if (conditional -> test_expression -> IsConstant())
        {
            //
            // branch_if(true?a:b, cond, lab) => branch_if(a, cond, lab);
            // branch_if(false?a:b, cond, lab) => branch_if(b, cond, lab);
            //
            EmitBranchIfExpression((IsZero(conditional -> test_expression)
                                    ? conditional -> false_expression
                                    : conditional -> true_expression),
                                   cond, lab, over);
        }
        else if (IsOne(conditional -> true_expression))
        {
            //
            // branch_if(expr?true:true, c, l) => expr, branch if c
            // branch_if(expr?true:false, c, l) => branch_if(expr, c, l);
            // branch_if(expr?true:b, c, l) => branch_if(expr || b, c, l);
            //
            if (IsOne(conditional -> false_expression))
            {
                EmitExpression(conditional -> test_expression, false);
                if (cond)
                    EmitBranch(OP_GOTO, lab, over);
            }
            else if (IsZero(conditional -> false_expression))
            {
                EmitBranchIfExpression(conditional -> test_expression,
                                       cond, lab, over);
            }
            else if (cond)
            {
                EmitBranchIfExpression(conditional -> test_expression, true,
                                       lab, over);
                EmitBranchIfExpression(conditional -> false_expression, true,
                                       lab, over);
            }
            else
            {
                Label skip;
                EmitBranchIfExpression(conditional -> test_expression, true,
                                       skip, over);
                EmitBranchIfExpression(conditional -> false_expression, false,
                                       lab, over);
                DefineLabel(skip);
                CompleteLabel(skip);
            }
        }
        else if (IsZero(conditional -> true_expression))
        {
            //
            // branch_if(expr?false:true, c, l) => branch_if(expr, ! c, l);
            // branch_if(expr?false:false, c, l) => expr, branch if ! c
            // branch_if(expr?false:b, c, l) => branch_if(!expr && b, c, l);
            //
            if (IsOne(conditional -> false_expression))
            {
                EmitBranchIfExpression(conditional -> test_expression,
                                       ! cond, lab, over);
            }
            else if (IsZero(conditional -> false_expression))
            {
                EmitExpression(conditional -> test_expression, false);
                if (! cond)
                    EmitBranch(OP_GOTO, lab, over);
            }
            else if (! cond)
            {
                EmitBranchIfExpression(conditional -> test_expression, true,
                                       lab, over);
                EmitBranchIfExpression(conditional -> false_expression, false,
                                       lab, over);
            }
            else
            {
                Label skip;
                EmitBranchIfExpression(conditional -> test_expression, true,
                                       skip, over);
                EmitBranchIfExpression(conditional -> false_expression, true,
                                       lab, over);
                DefineLabel(skip);
                CompleteLabel(skip);
            }
        }
        else if (IsOne(conditional -> false_expression))
        {
            //
            // branch_if(expr?a:true, c, l) => branch_if(!expr || a, c, l);
            //
            if (cond)
            {
                EmitBranchIfExpression(conditional -> test_expression, false,
                                       lab, over);
                EmitBranchIfExpression(conditional -> true_expression, true,
                                       lab, over);
            }
            else
            {
                Label skip;
                EmitBranchIfExpression(conditional -> test_expression, false,
                                       skip, over);
                EmitBranchIfExpression(conditional -> true_expression, false,
                                       lab, over);
                DefineLabel(skip);
                CompleteLabel(skip);
            }
        }
        else if (IsZero(conditional -> false_expression))
        {
            //
            // branch_if(expr?a:false, c, l) => branch_if(expr && a, c, l);
            //
            if (! cond)
            {
                EmitBranchIfExpression(conditional -> test_expression, false,
                                       lab, over);
                EmitBranchIfExpression(conditional -> true_expression, false,
                                       lab, over);
            }
            else
            {
                Label skip;
                EmitBranchIfExpression(conditional -> test_expression, false,
                                       skip, over);
                EmitBranchIfExpression(conditional -> true_expression, true,
                                       lab, over);
                DefineLabel(skip);
                CompleteLabel(skip);
            }
        }
        else
        {
            //
            // branch_if(expr?a:b, c, l) =>
            //   branch_if(expr, false, lab1)
            //   branch_if(a, c, l)
            //   goto lab2
            //   lab1: branch_if(b, c, l)
            //   lab2:
            //
            Label lab1, lab2;
            EmitBranchIfExpression(conditional -> test_expression, false, lab1,
                                   over);
            EmitBranchIfExpression(conditional -> true_expression, cond, lab,
                                   over);
            EmitBranch(OP_GOTO, lab2, over);
            DefineLabel(lab1);
            CompleteLabel(lab1);
            EmitBranchIfExpression(conditional -> false_expression, cond, lab,
                                   over);
            DefineLabel(lab2);
            CompleteLabel(lab2);
        }
        return;
    }

    AstInstanceofExpression* instanceof = p -> InstanceofExpressionCast();
    if (instanceof)
    {
        AstExpression* expr = StripNops(instanceof -> expression);
        TypeSymbol* left_type = expr -> Type();
        TypeSymbol* right_type = instanceof -> type -> symbol;
        if (right_type -> num_dimensions > 255)
        {
            semantic.ReportSemError(SemanticError::ARRAY_OVERFLOW,
                                    instanceof -> type);
        }
        if (left_type == control.null_type)
        {
            //
            // We know the result: false. But emit the left expression,
            // in case of side effects in (expr ? null : null).
            //
            EmitExpression(expr, false);
            if (! cond)
                EmitBranch(OP_GOTO, lab, over);
        }
        else if (expr -> IsConstant() || // a String constant
                 expr -> BinaryExpressionCast()) // a String concat
        {
            //
            // We know the result: true, since the expression is non-null
            // and String is a final class.
            //
            assert(left_type == control.String());
            EmitExpression(expr, false);
            if (cond)
                EmitBranch(OP_GOTO, lab, over);
        }
        else if ((expr -> ThisExpressionCast() ||
                  expr -> SuperExpressionCast() ||
                  expr -> ClassLiteralCast() ||
                  expr -> ClassCreationExpressionCast() ||
                  expr -> ArrayCreationExpressionCast()) &&
                 left_type -> IsSubtype(right_type))
        {
            //
            // We know the result: true, since the expression is non-null.
            //
            EmitExpression(expr, false);
            if (cond)
                EmitBranch(OP_GOTO, lab, over);
        }
        else
        {
            EmitExpression(expr);
            PutOp(OP_INSTANCEOF);
            PutU2(RegisterClass(right_type));
            EmitBranch((cond ? OP_IFNE : OP_IFEQ), lab, over);
        }
        return;
    }

    //
    // dispose of non-binary expression case by just evaluating
    // operand and emitting appropiate test.
    //
    AstBinaryExpression* bp = p -> BinaryExpressionCast();
    if (! bp)
    {
        EmitExpression(p);
        EmitBranch((cond ? OP_IFNE : OP_IFEQ), lab, over);
        return;
    }

    //
    // Here if binary expression, so extract operands
    //
    AstExpression* left = StripNops(bp -> left_expression);
    AstExpression* right = StripNops(bp -> right_expression);

    TypeSymbol* left_type = left -> Type();
    TypeSymbol* right_type = right -> Type();
    switch (bp -> Tag())
    {
    case AstBinaryExpression::AND_AND:
        //
        // branch_if(true&&b, cond, lab) => branch_if(b, cond, lab);
        // branch_if(false&&b, cond, lab) => branch_if(false, cond, lab);
        //
        if (left -> IsConstant())
        {
            if (IsOne(left))
                EmitBranchIfExpression(right, cond, lab, over);
            else if (! cond)
                EmitBranch(OP_GOTO, lab, over);
        }
        //
        // branch_if(a&&true, cond, lab) => branch_if(a, cond, lab);
        // branch_if(a&&false, cond, lab) => emit(a), pop; for side effects
        //
        else if (right -> IsConstant())
        {
            if (IsOne(right))
                EmitBranchIfExpression(left, cond, lab, over);
            else
            {
                EmitExpression(left, false);
                if (! cond)
                    EmitBranch(OP_GOTO, lab, over);
            }
        }
        //
        // branch_if(a&&b, true, lab) =>
        //   branch_if(a,false,skip);
        //   branch_if(b,true,lab);
        //   skip:
        // branch_if(a&&b, false, lab) =>
        //   branch_if(a,false,lab);
        //   branch_if(b,false,lab);
        //
        else if (cond)
        {
            Label skip;
            EmitBranchIfExpression(left, false, skip, over);
            EmitBranchIfExpression(right, true, lab, over);
            DefineLabel(skip);
            CompleteLabel(skip);
        }
        else
        {
            EmitBranchIfExpression(left, false, lab, over);
            EmitBranchIfExpression(right, false, lab, over);
        }
        return;
    case AstBinaryExpression::OR_OR:
        //
        // branch_if(false||b, cond, lab) => branch_if(b, cond, lab);
        // branch_if(true||b, cond, lab) => branch_if(true, cond, lab);
        //
        if (left -> IsConstant())
        {
            if (IsZero(left))
                EmitBranchIfExpression(right, cond, lab, over);
            else if (cond)
                EmitBranch(OP_GOTO, lab, over);
        }
        //
        // branch_if(a||false, cond, lab) => branch_if(a, cond, lab);
        // branch_if(a||true, cond, lab) => emit(a), pop; for side effects
        //
        else if (right -> IsConstant())
        {
            if (IsZero(right))
                EmitBranchIfExpression(left, cond, lab, over);
            else
            {
                EmitExpression(left, false);
                if (cond)
                    EmitBranch(OP_GOTO, lab, over);
            }
        }
        //
        // branch_if(a||b,true,lab) =>
        //   branch_if(a,true,lab);
        //   branch_if(b,true,lab);
        // branch_if(a||b,false,lab) =>
        //   branch_if(a,true,skip);
        //   branch_if(b,false,lab);
        //   skip:
        //
        else if (cond)
        {
            EmitBranchIfExpression(left, true, lab, over);
            EmitBranchIfExpression(right, true, lab, over);
        }
        else
        {
            Label skip;
            EmitBranchIfExpression(left, true, skip, over);
            EmitBranchIfExpression(right, false, lab, over);
            DefineLabel(skip);
            CompleteLabel(skip);
        }
        return;
    case AstBinaryExpression::XOR: // ^ on booleans is equavalent to !=
        assert(left_type == control.boolean_type);
        // Fallthrough!
    case AstBinaryExpression::EQUAL_EQUAL:
    case AstBinaryExpression::NOT_EQUAL:
        //
        // One of the operands is null. We must evaluate both operands, to get
        // any side effects in (expr ? null : null).
        //
        if (left_type == control.null_type || right_type == control.null_type)
        {
            EmitExpression(left, left_type != control.null_type);
            EmitExpression(right, right_type != control.null_type);
            if (left_type == right_type)
            {
                if (cond == (bp -> Tag() == AstBinaryExpression::EQUAL_EQUAL))
                {
                    EmitBranch(OP_GOTO, lab, over);
                }
            }
            else
            {
                if (bp -> Tag() == AstBinaryExpression::EQUAL_EQUAL)
                    EmitBranch(cond ? OP_IFNULL : OP_IFNONNULL, lab, over);
                else EmitBranch(cond ? OP_IFNONNULL : OP_IFNULL, lab, over);
            }
            return;
        }

        //
        // One of the operands is true. Branch on the other.
        //
        if (left_type == control.boolean_type &&
            (IsOne(left) || IsOne(right)))
        {
            EmitBranchIfExpression(IsOne(left) ? right : left,
                                   cond == (bp -> Tag() == AstBinaryExpression::EQUAL_EQUAL),
                                   lab, over);
            return;
        }

        //
        // Both operands are integer.
        //
        if (control.IsSimpleIntegerValueType(left_type) ||
             left_type == control.boolean_type)
        {
            assert(control.IsSimpleIntegerValueType(right_type) ||
                   right_type == control.boolean_type);

            if (IsZero(left) || IsZero(right))
            {
                if (left_type == control.boolean_type)
                {
                    //
                    // One of the operands is false. Branch on the other.
                    //
                    EmitBranchIfExpression(IsZero(left) ? right : left,
                                           cond == (bp -> Tag() != AstBinaryExpression::EQUAL_EQUAL),
                                           lab, over);
                }
                else
                {
                    //
                    // One of the operands is zero. Only emit the other.
                    //
                    EmitExpression(IsZero(left) ? right : left);

                    if (bp -> Tag() == AstBinaryExpression::EQUAL_EQUAL)
                        EmitBranch((cond ? OP_IFEQ : OP_IFNE), lab, over);
                    else EmitBranch((cond ? OP_IFNE : OP_IFEQ), lab, over);
                }
            }
            else
            {
                EmitExpression(left);
                EmitExpression(right);

                if (bp -> Tag() == AstBinaryExpression::EQUAL_EQUAL)
                    EmitBranch((cond ? OP_IF_ICMPEQ : OP_IF_ICMPNE), lab, over);
                else
                    EmitBranch((cond ? OP_IF_ICMPNE : OP_IF_ICMPEQ), lab, over);
            }

            return;
        }

        //
        // Both operands are reference types: just do the comparison.
        //
        if (IsReferenceType(left_type))
        {
            assert(IsReferenceType(right_type));
            EmitExpression(left);
            EmitExpression(right);

            if (bp -> Tag() == AstBinaryExpression::EQUAL_EQUAL)
                EmitBranch((cond ? OP_IF_ACMPEQ : OP_IF_ACMPNE), lab, over);
            else EmitBranch((cond ? OP_IF_ACMPNE : OP_IF_ACMPEQ), lab, over);

            return;
        }

        break;
    case AstBinaryExpression::IOR:
        //
        // One argument is false. Branch on other.
        //
        if (IsZero(left) || IsZero(right))
        {
            EmitBranchIfExpression(IsZero(left) ? right : left,
                                   cond, lab, over);
            return;
        }

        //
        // One argument is true. Emit the other, and result is true.
        //
        if (IsOne(left) || IsOne(right))
        {
            EmitExpression(IsOne(left) ? right : left, false);
            if (cond)
                EmitBranch(OP_GOTO, lab, over);
            return;
        }
        break;
    case AstBinaryExpression::AND:
        //
        // One argument is true. Branch on other.
        //
        if (IsOne(left) || IsOne(right))
        {
            EmitBranchIfExpression(IsOne(left) ? right : left,
                                   cond, lab, over);
            return;
        }

        //
        // One argument is false. Emit the other, and result is false.
        //
        if (IsZero(left) || IsZero(right))
        {
            EmitExpression(IsZero(left) ? right : left, false);
            if (! cond)
                EmitBranch(OP_GOTO, lab, over);
            return;
        }
        break;
    default:
        break;
    }

    //
    // here if not comparison, comparison for non-integral numeric types, or
    // integral comparison for which no special casing needed.
    // Begin by dealing with non-comparisons
    //
    switch (bp -> Tag())
    {
    case AstBinaryExpression::LESS:
    case AstBinaryExpression::LESS_EQUAL:
    case AstBinaryExpression::GREATER:
    case AstBinaryExpression::GREATER_EQUAL:
    case AstBinaryExpression::EQUAL_EQUAL:
    case AstBinaryExpression::NOT_EQUAL:
        break; // break to continue comparison processing
    default:
        //
        // not a comparison, get the (necessarily boolean) value
        // of the expression and branch on the result
        //
        EmitExpression(p);
        EmitBranch(cond ? OP_IFNE : OP_IFEQ, lab, over);
        return;
    }

    //
    //
    //
    Opcode opcode = OP_NOP,
           op_true,
           op_false;
    assert(left_type != control.boolean_type);
    if (control.IsSimpleIntegerValueType(left_type))
    {
        //
        // we have already dealt with EQUAL_EQUAL and NOT_EQUAL for the case
        // of two integers, but still need to look for comparisons for which
        // one operand may be zero.
        //
        if (IsZero(left))
        {
            EmitExpression(right);
            switch (bp -> Tag())
            {
            case AstBinaryExpression::LESS:
                // if (0 < x) same as  if (x > 0)
                op_true = OP_IFGT;
                op_false = OP_IFLE;
                break;
            case AstBinaryExpression::LESS_EQUAL:
                // if (0 <= x) same as if (x >= 0)
                op_true = OP_IFGE;
                op_false = OP_IFLT;
                break;
            case AstBinaryExpression::GREATER:
                // if (0 > x) same as if (x < 0)
                op_true = OP_IFLT;
                op_false = OP_IFGE;
                break;
            case AstBinaryExpression::GREATER_EQUAL:
                // if (0 >= x) same as if (x <= 0)
                op_true = OP_IFLE;
                op_false = OP_IFGT;
                break;
            default:
                assert(false);
                break;
            }
        }
        else if (IsZero(right))
        {
            EmitExpression(left);

            switch (bp -> Tag())
            {
            case AstBinaryExpression::LESS:
                op_true = OP_IFLT;
                op_false = OP_IFGE;
                break;
            case AstBinaryExpression::LESS_EQUAL:
                op_true = OP_IFLE;
                op_false = OP_IFGT;
                break;
            case AstBinaryExpression::GREATER:
                op_true = OP_IFGT;
                op_false = OP_IFLE;
                break;
            case AstBinaryExpression::GREATER_EQUAL:
                op_true = OP_IFGE;
                op_false = OP_IFLT;
                break;
            default:
                assert(false);
                break;
            }
        }
        else
        {
            EmitExpression(left);
            EmitExpression(right);

            switch (bp -> Tag())
            {
            case AstBinaryExpression::LESS:
                op_true = OP_IF_ICMPLT;
                op_false = OP_IF_ICMPGE;
                break;
            case AstBinaryExpression::LESS_EQUAL:
                op_true = OP_IF_ICMPLE;
                op_false = OP_IF_ICMPGT;
                break;
            case AstBinaryExpression::GREATER:
                op_true = OP_IF_ICMPGT;
                op_false = OP_IF_ICMPLE;
                break;
            case AstBinaryExpression::GREATER_EQUAL:
                op_true = OP_IF_ICMPGE;
                op_false = OP_IF_ICMPLT;
                break;
            default:
                assert(false);
                break;
            }
        }
    }
    else if (left_type == control.long_type)
    {
        EmitExpression(left);
        EmitExpression(right);

        opcode = OP_LCMP;

        //
        // branch according to result value on stack
        //
        switch (bp -> Tag())
        {
        case AstBinaryExpression::EQUAL_EQUAL:
            op_true = OP_IFEQ;
            op_false = OP_IFNE;
            break;
        case AstBinaryExpression::NOT_EQUAL:
            op_true = OP_IFNE;
            op_false = OP_IFEQ;
            break;
        case AstBinaryExpression::LESS:
            op_true = OP_IFLT;
            op_false = OP_IFGE;
            break;
        case AstBinaryExpression::LESS_EQUAL:
            op_true = OP_IFLE;
            op_false = OP_IFGT;
            break;
        case AstBinaryExpression::GREATER:
            op_true = OP_IFGT;
            op_false = OP_IFLE;
            break;
        case AstBinaryExpression::GREATER_EQUAL:
            op_true = OP_IFGE;
            op_false = OP_IFLT;
            break;
        default:
            assert(false);
            break;
        }
    }
    else if (left_type == control.float_type)
    {
        EmitExpression(left);
        EmitExpression(right);

        switch (bp -> Tag())
        {
        case AstBinaryExpression::EQUAL_EQUAL:
            opcode = OP_FCMPL;
            op_true = OP_IFEQ;
            op_false = OP_IFNE;
            break;
        case AstBinaryExpression::NOT_EQUAL:
            opcode = OP_FCMPL;
            op_true = OP_IFNE;
            op_false = OP_IFEQ;
            break;
        case AstBinaryExpression::LESS:
            opcode = OP_FCMPG;
            op_true = OP_IFLT;
            op_false = OP_IFGE;
            break;
        case AstBinaryExpression::LESS_EQUAL:
            opcode = OP_FCMPG;
            op_true = OP_IFLE;
            op_false = OP_IFGT;
            break;
        case AstBinaryExpression::GREATER:
            opcode = OP_FCMPL;
            op_true = OP_IFGT;
            op_false = OP_IFLE;
            break;
        case AstBinaryExpression::GREATER_EQUAL:
            opcode = OP_FCMPL;
            op_true = OP_IFGE;
            op_false = OP_IFLT;
            break;
        default:
            assert(false);
            break;
        }
    }
    else if (left_type == control.double_type)
    {
        EmitExpression(left);
        EmitExpression(right);
        switch (bp -> Tag())
        {
        case AstBinaryExpression::EQUAL_EQUAL:
            opcode = OP_DCMPL;
            op_true = OP_IFEQ;
            op_false = OP_IFNE;
            break;
        case AstBinaryExpression::NOT_EQUAL:
            opcode = OP_DCMPL;
            op_true = OP_IFNE;
            op_false = OP_IFEQ;
            break;
        case AstBinaryExpression::LESS:
            opcode = OP_DCMPG;
            op_true = OP_IFLT;
            op_false = OP_IFGE;
            break;
        case AstBinaryExpression::LESS_EQUAL:
            opcode = OP_DCMPG;
            op_true = OP_IFLE;
            op_false = OP_IFGT;
            break;
        case AstBinaryExpression::GREATER:
            opcode = OP_DCMPL;
            op_true = OP_IFGT;
            op_false = OP_IFLE;
            break;
        case AstBinaryExpression::GREATER_EQUAL:
            opcode = OP_DCMPL;
            op_true = OP_IFGE;
            op_false = OP_IFLT;
            break;
        default:
            assert(false);
            break;
        }
    }
    else assert(false && "comparison of unsupported type");

    if (opcode != OP_NOP)
        PutOp(opcode); // if need to emit comparison before branch

    EmitBranch (cond ? op_true : op_false, lab, over);
}


//
// Emits a synchronized statement, including monitor cleanup. The return
// value is true if the contained statement is abrupt.
//
bool ByteCode::EmitSynchronizedStatement(AstSynchronizedStatement* statement)
{
    int variable_index =
        method_stack -> TopBlock() -> block_symbol -> helper_variable_index;

    Label start_label;
    //
    // This code must be careful of asynchronous exceptions. Even if the
    // synchronized block is empty, user code can use Thread.stop(Throwable),
    // so we must ensure the monitor exits. We make sure that all instructions
    // after the monitorenter are covered.  By sticking the catchall code
    // before the synchronized block, we can even make abrupt exits inside the
    // statement be asynch-exception safe.  Note that the user can cause
    // deadlock (ie. an infinite loop), by releasing the monitor (via JNI or
    // some other means) in the block statement, so that the monitorexit fails
    // synchronously with an IllegalMonitorStateException and tries again; but
    // JLS 17.13 states that the compiler need not worry about such user
    // stupidity.
    //
    EmitBranch(OP_GOTO, start_label, NULL);
    u2 handler_pc = code_attribute -> CodeLength();
    assert(stack_depth == 0);
    stack_depth = 1; // account for the exception already on the stack
    LoadLocal(variable_index, control.Object()); // reload monitor
    PutOp(OP_MONITOREXIT);
    u2 throw_pc = code_attribute -> CodeLength();
    PutOp(OP_ATHROW);
    code_attribute -> AddException(handler_pc, throw_pc, handler_pc, 0);

    //
    // Even if enclosed statement is a nop, we must enter the monitor, because
    // of memory flushing side effects of synchronization.
    //
    DefineLabel(start_label);
    CompleteLabel(start_label);
    EmitExpression(statement -> expression);
    PutOp(OP_DUP); // duplicate for saving, entering monitor
    StoreLocal(variable_index, control.Object()); // save address of object
    PutOp(OP_MONITORENTER); // enter monitor associated with object

    assert(method_stack -> TopHandlerRangeStart().Length() == 0 &&
           method_stack -> TopHandlerRangeEnd().Length() == 0);
    method_stack -> TopHandlerRangeStart().Push(code_attribute -> CodeLength());
    bool abrupt = EmitBlockStatement(statement -> block);

    if (! abrupt)
    {
        LoadLocal(variable_index, control.Object()); // reload monitor
        PutOp(OP_MONITOREXIT);
    }
    u2 end_pc = code_attribute -> CodeLength();
    method_stack -> TopHandlerRangeEnd().Push(end_pc);
    unsigned count = method_stack -> TopHandlerRangeStart().Length();
    assert(count == method_stack -> TopHandlerRangeEnd().Length());
    while (count--)
    {
        code_attribute ->
            AddException(method_stack -> TopHandlerRangeStart().Pop(),
                         method_stack -> TopHandlerRangeEnd().Pop(),
                         handler_pc, 0);
    }
    return abrupt;
}


void ByteCode::EmitAssertStatement(AstAssertStatement* assertion)
{
    //
    // When constant true, the assert statement is a no-op.
    // Otherwise, assert a : b; is syntactic sugar for:
    //
    // while (! ($noassert && (a)))
    //     throw new java.lang.AssertionError(b);
    //
    if (semantic.IsConstantTrue(assertion -> condition) ||
        control.option.noassert ||
        control.option.target < JopaOption::SDK1_4)
    {
        return;
    }
    PutOp(OP_GETSTATIC);
    PutU2(RegisterFieldref(assertion -> assert_variable));
    Label label;
    EmitBranch(OP_IFNE, label);
    EmitBranchIfExpression(assertion -> condition, true, label);
    PutOp(OP_NEW);
    PutU2(RegisterClass(control.AssertionError()));
    PutOp(OP_DUP);

    MethodSymbol* constructor = NULL;
    if (assertion -> message_opt)
    {
        EmitExpression(assertion -> message_opt);
        TypeSymbol* type = assertion -> message_opt -> Type();
        if (! control.AssertionError() -> Bad())
        {
            // We found the class, now can we find the method?
            if (type == control.char_type)
                constructor = control.AssertionError_InitWithCharMethod();
            else if (type == control.boolean_type)
                constructor = control.AssertionError_InitWithBooleanMethod();
            else if (type == control.int_type || type == control.short_type ||
                     type == control.byte_type)
            {
                constructor = control.AssertionError_InitWithIntMethod();
            }
            else if (type == control.long_type)
                constructor = control.AssertionError_InitWithLongMethod();
            else if (type == control.float_type)
                constructor = control.AssertionError_InitWithFloatMethod();
            else if (type == control.double_type)
                constructor = control.AssertionError_InitWithDoubleMethod();
            else if (type == control.null_type || IsReferenceType(type))
                constructor = control.AssertionError_InitWithObjectMethod();
            else assert (false && "Missing AssertionError constructor!");
            if (! constructor) // We didn't find it; suckage....
                // TODO: error ought to include what we were looking for
                semantic.ReportSemError(SemanticError::LIBRARY_METHOD_NOT_FOUND,
                                        assertion,
                                        unit_type -> ContainingPackageName(),
                                        unit_type -> ExternalName());
        }
        else
        {
            // The type for AssertionError is BAD, that means it wasn't
            // found! but the calls to control.AssertionError() above will
            // file a semantic error for us, no need to here.
        }
        ChangeStack(- GetTypeWords(type));
    }
    else constructor = control.AssertionError_InitMethod();

    PutOp(OP_INVOKESPECIAL);
    PutU2(RegisterLibraryMethodref(constructor));
    PutOp(OP_ATHROW);
    DefineLabel(label);
    CompleteLabel(label);
}


void ByteCode::EmitForeachStatement(AstForeachStatement* foreach)
{
    int helper_index =
        method_stack -> TopBlock() -> block_symbol -> helper_variable_index;
    bool abrupt;
    EmitExpression(foreach -> expression);
    Label loop;
    Label& comp = method_stack -> TopContinueLabel();
    Label end;
    TypeSymbol* expr_type = foreach -> expression -> Type();
    VariableSymbol* var =
        foreach -> formal_parameter -> formal_declarator -> symbol;
    TypeSymbol* component_type = var -> Type();
    if (expr_type -> IsArray())
    {
        //
        // Turn 'l: for(a b : c) d' into
        // { expr_type #0 = c;
        //   int #1 = #0.length;
        //   l: for(int #2 = 0; #2 < #1; #2++) {
        //     a b = #0[#2];
        //     d; }}
        // Or in bytecode:
        // eval c onto stack
        // dup
        // astore helper_index
        // arraylength
        // dup
        // istore helper_index+1
        // ifeq end
        // iconst_0
        // istore helper_index+2
        // iconst_0
        // loop:
        // aload helper_index
        // swap
        // xaload (for x = b, s, i, l, c, f, d, a)
        // assignment-conversion (if necessary)
        // xstore b (for x = i, l, f, d, a)
        // eval d (continue to comp, break to end)
        // comp:
        // iinc helper_index+2, 1
        // iload helper_index+1
        // iload helper_index+2
        // dup_x1
        // if_icmpgt loop
        // pop
        // end:
        //
        TypeSymbol* expr_subtype = expr_type -> ArraySubtype();
        if (IsNop(foreach -> statement) &&
            (! component_type -> Primitive() || expr_subtype -> Primitive()))
        {
            //
            // Optimization (arrays only): no need to increment loop counter
            // if nothing is done in the loop; and we simply check that the
            // array is non-null from arraylength. But beware of autounboxing,
            // which can cause NullPointerException.
            //
            PutOp(OP_ARRAYLENGTH);
            PutOp(OP_POP);
            return;
        }
        PutOp(OP_DUP);
        StoreLocal(helper_index, expr_type);
        PutOp(OP_ARRAYLENGTH);
        PutOp(OP_DUP);
        StoreLocal(helper_index + 1, control.int_type);
        EmitBranch(OP_IFEQ, end);
        PutOp(OP_ICONST_0);
        StoreLocal(helper_index + 2, control.int_type);
        PutOp(OP_ICONST_0);
        DefineLabel(loop);
        LoadLocal(helper_index, expr_type);
        PutOp(OP_SWAP);
        LoadArrayElement(expr_type -> ArraySubtype());
        EmitCast(component_type, expr_type -> ArraySubtype());
        u2 var_pc = code_attribute -> CodeLength();        
        StoreLocal(var -> LocalVariableIndex(), component_type);
        abrupt = EmitStatement(foreach -> statement);
        if (control.option.g & JopaOption::VARS)
        {
            local_variable_table_attribute ->
                AddLocalVariable(var_pc, code_attribute -> CodeLength(),
                                 RegisterName(var -> ExternalIdentity()),
                                 RegisterUtf8(component_type -> signature),
                                 var -> LocalVariableIndex());
        }
        if (! abrupt || foreach -> statement -> can_complete_normally)
        {
            DefineLabel(comp);
            PutOpIINC(helper_index + 2, 1);
            LoadLocal(helper_index + 1, control.int_type);
            LoadLocal(helper_index + 2, control.int_type);
            PutOp(OP_DUP_X1);
            EmitBranch(OP_IF_ICMPGT, loop);
            PutOp(OP_POP);
        }
    }
    else
    {
        assert(foreach -> expression -> Type() ->
               IsSubtype(control.Iterable()));
        //
        // Turn 'l: for(a b : c) d' into
        // for(java.util.Iterator #0 = c.iterator(); #0.hasNext();) {
        //   a b = (a) c.next();
        //   d; }
        // Or in bytecode:
        // eval c onto stack
        // invokeinterface java.lang.Iterable.iterator()Ljava/util/Iterator;
        // dup
        // invokeinterface java.util.Iterator.hasNext()Z
        // ifeq cleanup
        // dup
        // astore helper_index
        // loop:
        // invokeinterface java.util.Iterator.next()Ljava/lang/Object;
        // checkcast a
        // astore b
        // eval d (continue to comp, break to end)
        // comp:
        // aload helper_index
        // dup
        // invokeinterface java.util.Iterator.hasNext()Z
        // ifne loop
        // cleanup:
        // pop
        // end:
        //
        Label cleanup;
        PutOp(OP_INVOKEINTERFACE);
        PutU2(RegisterLibraryMethodref(control.Iterable_iteratorMethod()));
        PutU1(1);
        PutU1(0);
        ChangeStack(1);
        PutOp(OP_DUP);
        PutOp(OP_INVOKEINTERFACE);
        u2 hasNext_index =
            RegisterLibraryMethodref(control.Iterator_hasNextMethod());
        PutU2(hasNext_index);
        PutU1(1);
        PutU1(0);
        ChangeStack(1);
        EmitBranch(OP_IFEQ, cleanup);
        PutOp(OP_DUP);
        StoreLocal(helper_index, control.Iterator());
        DefineLabel(loop);
        PutOp(OP_INVOKEINTERFACE);
        PutU2(RegisterLibraryMethodref(control.Iterator_nextMethod()));
        PutU1(1);
        PutU1(0);
        ChangeStack(1);
        if (component_type != control.Object())
        {
            PutOp(OP_CHECKCAST);
            PutU2(RegisterClass(component_type));
        }
        u2 var_pc = code_attribute -> CodeLength();        
        StoreLocal(var -> LocalVariableIndex(), component_type);
        abrupt = EmitStatement(foreach -> statement);
        if (control.option.g & JopaOption::VARS)
        {
            local_variable_table_attribute ->
                AddLocalVariable(var_pc, code_attribute -> CodeLength(),
                                 RegisterName(var -> ExternalIdentity()),
                                 RegisterUtf8(component_type -> signature),
                                 var -> LocalVariableIndex());
        }
        if (! abrupt || foreach -> statement -> can_complete_normally)
        {
            DefineLabel(comp);
            LoadLocal(helper_index, control.Iterator());
            PutOp(OP_DUP);
            PutOp(OP_INVOKEINTERFACE);
            PutU2(hasNext_index);
            PutU1(1);
            PutU1(0);
            ChangeStack(1);
            EmitBranch(OP_IFNE, loop);
        }
        else ChangeStack(1);
        DefineLabel(cleanup);
        CompleteLabel(cleanup);
        PutOp(OP_POP);
    }
    DefineLabel(end);
    CompleteLabel(loop);
    CompleteLabel(comp);
    CompleteLabel(end);
}


//
// JLS is Java Language Specification
// JVM is Java Virtual Machine
//
// Expressions: Chapter 14 of JLS
//
int ByteCode::EmitExpression(AstExpression* expression, bool need_value)
{
    expression = StripNops(expression);
    if (expression -> IsConstant())
    {
        if (need_value)
        {
            LoadLiteral(expression -> value, expression -> Type());
            return GetTypeWords(expression -> Type());
        }
        return 0;
    }

    switch (expression -> kind)
    {
    case Ast::NAME:
        return EmitName((AstName*) expression, need_value);
    case Ast::THIS_EXPRESSION:
        {
            AstThisExpression* this_expr = (AstThisExpression*) expression;
            if (this_expr -> resolution_opt && need_value)
                return EmitExpression(this_expr -> resolution_opt, true);
        }
        if (need_value)
        {
            PutOp(OP_ALOAD_0);
            return 1;
        }
        return 0;
    case Ast::SUPER_EXPRESSION:
        {
            AstSuperExpression* super_expr = (AstSuperExpression*) expression;
            if (super_expr -> resolution_opt && need_value)
                return EmitExpression(super_expr -> resolution_opt, true);
        }
        if (need_value)
        {
            PutOp(OP_ALOAD_0);
            return 1;
        }
        return 0;
    case Ast::CLASS_CREATION:
        return EmitClassCreationExpression
            ((AstClassCreationExpression*) expression, need_value);
    case Ast::ARRAY_CREATION:
        return EmitArrayCreationExpression((AstArrayCreationExpression*) expression, need_value);
    case Ast::CLASS_LITERAL:
        {
            AstClassLiteral* class_lit = (AstClassLiteral*) expression;
            if (class_lit -> resolution_opt)
                return GenerateClassAccess(class_lit, need_value);
            TypeSymbol* type = expression -> symbol -> TypeCast();
            if (type)
            {
                // Must load for side effect of class not found
                assert(type == control.Class());
                LoadConstantAtIndex(RegisterClass(class_lit -> type ->
                                                  symbol));
                if (! need_value)
                    PutOp(OP_POP);
            }
            else if (need_value)
            {
                // No side effects for Integer.TYPE and friends.
                assert(expression -> symbol -> VariableCast());
                PutOp(OP_GETSTATIC);
                PutU2(RegisterFieldref((VariableSymbol*) expression ->
                                       symbol));
            }
            return need_value ? 1 : 0;
        }
    case Ast::DOT:
        return EmitFieldAccess((AstFieldAccess*) expression, need_value);
    case Ast::CALL:
        return EmitMethodInvocation((AstMethodInvocation*) expression,
                                    need_value);
    case Ast::ARRAY_ACCESS:
        {
            // must evaluate, for potential Exception side effects
            int words = EmitArrayAccessRhs((AstArrayAccess*) expression);
            if (need_value)
                return words;
            PutOp(words == 1 ? OP_POP : OP_POP2);
            return 0;
        }
    case Ast::POST_UNARY:
        return EmitPostUnaryExpression((AstPostUnaryExpression*) expression,
                                       need_value);
    case Ast::PRE_UNARY:
        return EmitPreUnaryExpression((AstPreUnaryExpression*) expression,
                                      need_value);
    case Ast::CAST:
        return EmitCastExpression((AstCastExpression*) expression, need_value);
    case Ast::BINARY:
        return EmitBinaryExpression((AstBinaryExpression*) expression,
                                    need_value);
    case Ast::INSTANCEOF:
        return EmitInstanceofExpression((AstInstanceofExpression*) expression,
                                        need_value);
    case Ast::CONDITIONAL:
        return EmitConditionalExpression(((AstConditionalExpression*)
                                          expression),
                                         need_value);
    case Ast::ASSIGNMENT:
        return EmitAssignmentExpression((AstAssignmentExpression*) expression,
                                        need_value);
    case Ast::NULL_LITERAL:
        if (need_value)
        {
            PutOp(OP_ACONST_NULL);
            return 1;
        }
        return 0;
    default:
        assert(false && "unknown expression kind");
        break;
    }
    return 0; // even though we will not reach here
}


AstExpression* ByteCode::VariableExpressionResolution(AstExpression* expression)
{
    //
    // JLS2 added ability for parenthesized variable to remain a variable.
    // If the expression was resolved, get the resolution.
    //
    expression = StripNops(expression);
    AstFieldAccess* field = expression -> FieldAccessCast();
    if (field && field -> resolution_opt)
        return field -> resolution_opt;
    AstName* name = expression -> NameCast();
    if (name && name -> resolution_opt)
        return name -> resolution_opt;
    return expression;
}


TypeSymbol* ByteCode::VariableTypeResolution(AstExpression* expression,
                                             VariableSymbol* sym)
{
    expression = VariableExpressionResolution(expression);
    AstFieldAccess* field = expression -> FieldAccessCast();
    AstName* name = expression -> NameCast();
    assert(field || name);

    //
    // JLS2 13.1 Use the type of the base expression for qualified reference
    // (this even works for super expressions), and the innermost type that
    // contains the (possibly inherited) field for simple name reference.
    //
    // Prior to JDK 1.4, VMs incorrectly complained if a field declared in an
    // interface is referenced by inheritance, even though the JVMS permits it
    // and JLS 13 requires it.
    //
    TypeSymbol* candidate = field ? field -> base -> Type()
        : name -> base_opt ? name -> base_opt -> Type() : unit_type;
    return (sym -> ContainingType() -> ACC_INTERFACE() &&
            control.option.target < JopaOption::SDK1_4)
        ? sym -> ContainingType() : candidate;
}


TypeSymbol* ByteCode::MethodTypeResolution(AstExpression* base,
                                           MethodSymbol* msym)
{
    //
    // JLS 13.1 If the method is declared in Object, use Object. Otherwise,
    // use the type of the base expression for qualified reference (this even
    // works for super expressions), and the innermost type that contains the
    // (possibly inherited) method for simple name reference.  However, if
    // this is an accessor method, use the owner_type (since the base type
    // relates to the accessed expression, not the accessor method).
    //
    // Java 5: For static imports, base is NULL but we should use owner_type,
    // not unit_type, so the correct class is referenced in the bytecode.
    //
    TypeSymbol* owner_type = msym -> containing_type;
    TypeSymbol* base_type = msym -> ACC_SYNTHETIC() ? owner_type
        : base ? base -> Type()
        : msym -> ACC_STATIC() ? owner_type  // Java 5: static import
        : unit_type;
    return owner_type == control.Object() ? owner_type : base_type;
}


void ByteCode::EmitFieldAccessLhsBase(AstExpression* expression)
{
    expression = VariableExpressionResolution(expression);
    AstFieldAccess* field = expression -> FieldAccessCast();
    AstName* name = expression -> NameCast();

    //
    // We now have the right expression. Check if it is qualified, in which
    // case we process the base. Otherwise, it must be a simple name.
    //
    if (field || (name && name -> base_opt))
        EmitExpression(field ? field -> base : name -> base_opt);
    else PutOp(OP_ALOAD_0); // get address of "this"
}


void ByteCode::EmitFieldAccessLhs(AstExpression* expression)
{
    EmitFieldAccessLhsBase(expression);
    PutOp(OP_DUP);     // save base address of field for later store
    PutOp(OP_GETFIELD);
    if (control.IsDoubleWordType(expression -> Type()))
        ChangeStack(1);

    VariableSymbol* sym = (VariableSymbol*) expression -> symbol;
    PutU2(RegisterFieldref(VariableTypeResolution(expression, sym), sym));
}


//
// Generate code for access method used to set class literal fields, when
// compiling for older VMs.
//
void ByteCode::GenerateClassAccessMethod()
{
    assert(control.option.target < JopaOption::SDK1_5);
    //
    // Here, we add a line-number attribute entry for this method.
    // Even though this is a generated method, JPDA debuggers will
    // still fail setting breakpoints if methods don't have line numbers.
    // Sun's javac compiler generates a single line number entry
    // with start_pc set to zero and line number set to the first line of
    // code in the source. In testing, it appears that setting the start_pc
    // and line_number to zero as we do here, also works.
    //
    line_number_table_attribute -> AddLineNumber(0, 0);

    //
    // Since the VM does not have a nice way of finding a class without a
    // runtime object, we use this approach.  Notice that forName can throw
    // a checked exception, but JLS semantics do not allow this, so we must
    // add a catch block to convert the problem to an unchecked Error.
    // Likewise, note that we must not initialize the class in question,
    // hence the use of forName on array types in all cases.
    //
    // The generated code is semantically equivalent to:
    //
    // /*synthetic*/ static java.lang.Class class$(java.lang.String name,
    //                                             boolean array) {
    //     try {
    //         Class result = java.lang.Class.forName(name);
    //         return array ? result : result.getComponentType();
    //     } catch (ClassNotFoundException e) {
    //         throw new NoClassDefFoundError(((Throwable) e).getMessage());
    //     }
    // }
    //
    // When option.target >= SDK1_4, we use the new exception chaining,
    // and the catch clause becomes
    //   throw (Error) ((Throwable) new NoClassDefFoundError()).initCause(e);
    //
    // Since ClassNotFoundException inherits, rather than declares, getMessage,
    // we link to Throwable, and use the cast to Throwable in the code above to
    // show that we are still obeying JLS 13.1, which requires that .class
    // files must link to the type of the qualifying expression.
    //
    //  aload_0        load class name in array form
    //  invokestatic   java/lang/Class.forName(Ljava/lang/String;)Ljava/lang/Class;
    //  iload_1        load array
    //  ifne label
    //  invokevirtual  java/lang/Class.getComponentType()Ljava/lang/Class;
    //  label:
    //  areturn        return Class object
    //
    // pre-SDK1_4 exception handler if forName fails (optimization: the
    // ClassNotFoundException will already be on the stack):
    //
    //  invokevirtual  java/lang/Throwable.getMessage()Ljava/lang/String;
    //  new            java/lang/NoClassDefFoundError
    //  dup_x1         save copy to throw, but below string arg to constructor
    //  swap           swap string and new object to correct order
    //  invokespecial  java/lang/NoClassDefFoundError.<init>(Ljava/lang/String;)V
    //  athrow         throw the correct exception
    //
    // post-SDK1_4 exception handler if forName fails (optimization: the
    // ClassNotFoundException will already be on the stack):
    //
    //  new            java/lang/NoClassDefFoundError
    //  dup_x1         save copy, but below cause
    //  invokespecial  java/lang/NoClassDefFoundError.<init>()V
    //  invokevirtual  java/lang/Throwable.initCause(Ljava/lang/Throwable;)Ljava/lang/Throwable;
    //  athrow         throw the correct exception
    //
    Label label;
    PutOp(OP_ALOAD_0);
    PutOp(OP_INVOKESTATIC);
    PutU2(RegisterLibraryMethodref(control.Class_forNameMethod()));
    PutOp(OP_ILOAD_1);
    EmitBranch(OP_IFNE, label);
    PutOp(OP_INVOKEVIRTUAL);
    PutU2(RegisterLibraryMethodref(control.Class_getComponentTypeMethod()));
    ChangeStack(1); // account for the return
    DefineLabel(label);
    CompleteLabel(label);
    PutOp(OP_ARETURN);
    code_attribute ->
      AddException(0, 12, 12, RegisterClass(control.ClassNotFoundException()));

    ChangeStack(1); // account for the exception on the stack
    if (control.option.target < JopaOption::SDK1_4)
    {
        PutOp(OP_INVOKEVIRTUAL);
        PutU2(RegisterLibraryMethodref(control.Throwable_getMessageMethod()));
        ChangeStack(1); // account for the returned string
        PutOp(OP_NEW);
        PutU2(RegisterClass(control.NoClassDefFoundError()));
        PutOp(OP_DUP_X1);
        PutOp(OP_SWAP);
        PutOp(OP_INVOKESPECIAL);
        PutU2(RegisterLibraryMethodref(control.NoClassDefFoundError_InitStringMethod()));
        ChangeStack(-1); // account for the argument to the constructor
    }
    else
    {
        PutOp(OP_NEW);
        PutU2(RegisterClass(control.NoClassDefFoundError()));
        PutOp(OP_DUP_X1);
        PutOp(OP_INVOKESPECIAL);
        PutU2(RegisterLibraryMethodref(control.NoClassDefFoundError_InitMethod()));
        PutOp(OP_INVOKEVIRTUAL);
        PutU2(RegisterLibraryMethodref(control.Throwable_initCauseMethod()));
    }
    PutOp(OP_ATHROW);
}


//
// Generate bytecode for a bridge method.
//
// Bridge methods are synthetic methods generated for generic covariant overrides.
// They have the erased signature of the superclass method and simply delegate
// to the actual method with the specialized signature.
//
// Example:
//   class Box<T> { T get() {...} }
//   class StringBox extends Box<String> { String get() {...} }
//
// Bridge generated in StringBox:
//   /* bridge */ Object get() { return this.get(); }  // calls String version
//
void ByteCode::GenerateBridgeMethod(MethodSymbol* bridge)
{
    assert(bridge -> IsBridge());
    MethodSymbol* target = bridge -> BridgeTarget();
    assert(target);

    // Add minimal line number info for debuggers
    line_number_table_attribute -> AddLineNumber(0, 0);

    // Load 'this' if not static
    if (! bridge -> ACC_STATIC())
    {
        PutOp(OP_ALOAD_0);
    }

    // Load all parameters
    u2 local_index = bridge -> ACC_STATIC() ? 0 : 1;
    for (unsigned i = 0; i < bridge -> NumFormalParameters(); i++)
    {
        VariableSymbol* param = bridge -> FormalParameter(i);
        TypeSymbol* param_type = param -> Type();

        if (control.IsSimpleIntegerValueType(param_type) ||
            param_type == control.boolean_type)
        {
            LoadLocal(local_index, param_type);
            local_index++;
        }
        else if (param_type == control.long_type)
        {
            PutOp(OP_LLOAD);
            PutU1(local_index);
            local_index += 2;
        }
        else if (param_type == control.float_type)
        {
            PutOp(OP_FLOAD);
            PutU1(local_index);
            local_index++;
        }
        else if (param_type == control.double_type)
        {
            PutOp(OP_DLOAD);
            PutU1(local_index);
            local_index += 2;
        }
        else // reference type
        {
            LoadLocal(local_index, param_type);
            local_index++;
        }
    }

    // Invoke the actual method
    if (bridge -> ACC_STATIC())
    {
        PutOp(OP_INVOKESTATIC);
        PutU2(RegisterMethodref(target -> containing_type, target));
    }
    else if (target -> containing_type -> ACC_INTERFACE())
    {
        PutOp(OP_INVOKEINTERFACE);
        PutU2(RegisterMethodref(target -> containing_type, target));
        PutU1(target -> NumFormalParameters() + 1); // +1 for 'this'
        PutU1(0);
    }
    else
    {
        PutOp(OP_INVOKEVIRTUAL);
        PutU2(RegisterMethodref(target -> containing_type, target));
    }

    // Return the result
    TypeSymbol* return_type = bridge -> Type();
    if (return_type == control.void_type)
    {
        PutOp(OP_RETURN);
    }
    else if (control.IsSimpleIntegerValueType(return_type) ||
             return_type == control.boolean_type)
    {
        PutOp(OP_IRETURN);
    }
    else if (return_type == control.long_type)
    {
        PutOp(OP_LRETURN);
    }
    else if (return_type == control.float_type)
    {
        PutOp(OP_FRETURN);
    }
    else if (return_type == control.double_type)
    {
        PutOp(OP_DRETURN);
    }
    else // reference type
    {
        // No explicit cast needed - the JVM handles covariant returns
        PutOp(OP_ARETURN);
    }
}


//
// Generate enum values() method:
// public static EnumType[] values() {
//     EnumType[] arr = new EnumType[N];
//     arr[0] = CONSTANT0;
//     arr[1] = CONSTANT1;
//     ...
//     return arr;
// }
//
void ByteCode::GenerateEnumValuesMethod()
{
    line_number_table_attribute -> AddLineNumber(0, 0);

    // Find enum declaration to get enum constants
    AstClassBody* class_body = unit_type -> declaration;
    if (!class_body || !class_body -> owner)
    {
        PutOp(OP_ACONST_NULL);  // Auto-tracked: +1
        PutOp(OP_ARETURN);       // Auto-tracked: -1
        return;
    }

    AstEnumDeclaration* enum_decl = class_body -> owner -> EnumDeclarationCast();
    if (!enum_decl)
    {
        PutOp(OP_ACONST_NULL);  // Auto-tracked: +1
        PutOp(OP_ARETURN);       // Auto-tracked: -1
        return;
    }

    unsigned num_constants = enum_decl -> NumEnumConstants();

    // Create array: new EnumType[N]
    LoadImmediateInteger(num_constants);  // Auto-tracked: +1
    PutOp(OP_ANEWARRAY);  // Auto-tracked: 0 (pops 1, pushes 1)
    PutU2(RegisterClass(unit_type));

    // Populate array with enum constants
    for (unsigned i = 0; i < num_constants; i++)
    {
        AstEnumConstant* enum_constant = enum_decl -> EnumConstant(i);
        VariableSymbol* field = enum_constant -> field_symbol;

        if (field)
        {
            PutOp(OP_DUP);  // Auto-tracked: +1
            LoadImmediateInteger(i);  // Auto-tracked: +1
            PutOp(OP_GETSTATIC);  // Auto-tracked: +1
            PutU2(RegisterFieldref(field));
            PutOp(OP_AASTORE);  // Auto-tracked: -3
        }
    }

    // Stack now has just the array
    PutOp(OP_ARETURN);  // Auto-tracked: -1
}


//
// Generate enum valueOf(String) method:
// public static EnumType valueOf(String name) {
//     return (EnumType) Enum.valueOf(EnumType.class, name);
// }
//
void ByteCode::GenerateEnumValueOfMethod()
{
    line_number_table_attribute -> AddLineNumber(0, 0);

    // ldc EnumType.class (load class literal)
    LoadConstantAtIndex(RegisterClass(unit_type));  // Auto-tracked: +1

    // aload_0 (load String parameter)
    PutOp(OP_ALOAD_0);  // Auto-tracked: +1

    // invokestatic Enum.valueOf(Class, String)
    TypeSymbol* enum_class = control.Enum();
    NameSymbol* valueOf_name = control.FindOrInsertName(L"valueOf", 7);

    // Find valueOf method in Enum class
    MethodSymbol* enum_valueOf = NULL;
    for (unsigned i = 0; i < enum_class -> NumMethodSymbols(); i++)
    {
        MethodSymbol* method = enum_class -> MethodSym(i);
        if (method -> Identity() == valueOf_name &&
            method -> NumFormalParameters() == 2 &&
            method -> ACC_STATIC())
        {
            enum_valueOf = method;
            break;
        }
    }

    if (enum_valueOf)
    {
        // INVOKESTATIC doesn't auto-track properly for static methods with returns
        ChangeStack(-2); // Pop 2 arguments (Class, String)
        PutOp(OP_INVOKESTATIC);
        PutU2(RegisterMethodref(enum_class, enum_valueOf));
        ChangeStack(1); // Push return value (Enum)
    }
    else
    {
        // If valueOf not found, just return null
        PutOp(OP_POP);  // Auto-tracked: -1
        PutOp(OP_POP);  // Auto-tracked: -1
        PutOp(OP_ACONST_NULL);  // Auto-tracked: +1
    }

    // checkcast to EnumType
    PutOp(OP_CHECKCAST);  // Auto-tracked: 0 (pops 1, pushes 1)
    PutU2(RegisterClass(unit_type));

    // areturn
    PutOp(OP_ARETURN);  // Auto-tracked: -1
}


//
// Emit boxing conversion: primitive → wrapper
// Call: WrapperClass.valueOf(primitive)
// Stack: primitive → wrapper
//
void ByteCode::EmitBoxingConversion(TypeSymbol* source_type, TypeSymbol* target_type)
{
    // Verify this is actually a boxing conversion
    if (!source_type || !target_type || !source_type -> Primitive())
        return;

    // Find the wrapper type and valueOf method
    TypeSymbol* wrapper_type = semantic.GetWrapperType(source_type);
    if (!wrapper_type || wrapper_type != target_type)
        return;

    MethodSymbol* valueOf_method = semantic.GetBoxingMethod(source_type);
    if (!valueOf_method)
        return;

    // Call WrapperClass.valueOf(primitive)
    // Primitive value is already on stack
    // INVOKESTATIC doesn't auto-track stack, so we manually manage it
    int arg_words = GetTypeWords(source_type);
    ChangeStack(-arg_words); // Pop the primitive argument
    PutOp(OP_INVOKESTATIC);
    PutU2(RegisterMethodref(wrapper_type, valueOf_method));
    ChangeStack(1); // Push the wrapper object reference (always 1 word)
}


//
// Emit unboxing conversion: wrapper → primitive
// Call: wrapper.primitiveValue()
// Stack: wrapper → primitive
//
void ByteCode::EmitUnboxingConversion(TypeSymbol* source_type, TypeSymbol* target_type)
{
    // Verify this is actually an unboxing conversion
    if (!source_type || !target_type || !target_type -> Primitive())
        return;

    // Find the primitive type and unboxing method
    TypeSymbol* primitive_type = semantic.GetPrimitiveType(source_type);
    if (!primitive_type || primitive_type != target_type)
        return;

    MethodSymbol* unbox_method = semantic.GetUnboxingMethod(source_type);
    if (!unbox_method)
        return;

    // Call wrapper.primitiveValue()
    // Wrapper object is already on stack
    // INVOKEVIRTUAL automatically pops 'this' (-1), so we only adjust for return value
    PutOp(OP_INVOKEVIRTUAL);
    PutU2(RegisterMethodref(source_type, unbox_method));
    int result_words = GetTypeWords(target_type);
    ChangeStack(result_words); // Push the primitive result
}


//
// Generate code to dymanically initialize the field for a class literal, and
// return its value. Only generated for older VMs (since newer ones support
// ldc class).
//
int ByteCode::GenerateClassAccess(AstClassLiteral* class_lit,
                                  bool need_value)
{
    assert(control.option.target < JopaOption::SDK1_5);
    //
    // Evaluate X.class literal. If X is a primitive type, this is a
    // predefined field, and we emitted it directly rather than relying on
    // this method. Otherwise, we have created a synthetic field to cache
    // the desired result, and we initialize it at runtime. Within a class,
    // this cannot be done in the static initializer, because it is possible
    // to access a class literal before a class is initialized.
    //
    // Foo.Bar.class becomes
    // (class$Foo$Bar == null ? class$Foo$Bar = class$("[LFoo.Bar;", false)
    //                        : class$Foo$Bar)
    // int[].class becomes
    // (array$I == null ? array$I = class$("[I", true) : array$I)
    //
    // getstatic class_field     load class field
    // dup                       optimize: common case is non-null
    // ifnonnull label           branch if it exists, otherwise initialize
    // pop                       pop the null we just duplicated
    // load class_constant       get name of class
    // iconst_x                  true iff array
    // invokestatic              invoke synthetic class$ method
    // dup                       save value so can return it
    // put class_field           initialize the field
    // label:
    //
    Label label;
    assert(class_lit -> symbol -> VariableCast());
    VariableSymbol* cache = (VariableSymbol*) class_lit -> symbol;

    u2 field_index = RegisterFieldref(cache);

    PutOp(OP_GETSTATIC);
    PutU2(field_index);
    if (need_value)
        PutOp(OP_DUP);
    EmitBranch(OP_IFNONNULL, label);

    if (need_value)
        PutOp(OP_POP);
    TypeSymbol* type = class_lit -> type -> symbol;
    if (type -> num_dimensions > 255)
        semantic.ReportSemError(SemanticError::ARRAY_OVERFLOW, class_lit);
    bool is_array = type -> IsArray();
    if (! is_array)
        type = type -> GetArrayType(control.system_semantic, 1);
    LoadLiteral(type -> FindOrInsertClassLiteralName(control),
                control.String());
    PutOp(is_array ? OP_ICONST_1 : OP_ICONST_0);
    PutOp(OP_INVOKESTATIC);
    CompleteCall(cache -> ContainingType() -> ClassLiteralMethod(), 2);
    if (need_value)
        PutOp(OP_DUP);
    PutOp(OP_PUTSTATIC);
    PutU2(field_index);
    DefineLabel(label);
    CompleteLabel(label);
    return need_value ? 1 : 0;
}


//
// Generate code for initializing assert variable
//
void ByteCode::GenerateAssertVariableInitializer(TypeSymbol* tsym,
                                                 VariableSymbol* vsym)
{
    //
    // Create the field initializer. This approach avoids using a class
    // literal, for two reasons:
    //   - we use fewer bytecodes if the rest of the class does not use class
    //     literals (and we need no try-catch block)
    //   - determining assertion status will not initialize an enclosing class.
    //
    // Unfortunately, until the VM supports easier determination of classes
    // from a static context, we must create an empty garbage array.
    // We initialize to the opposite of desiredAssertionStatus to obey the
    // semantics of assert - until class initialization starts, the default
    // value of false in this variable will enable asserts anywhere in the
    // class.
    //
    // private static final boolean $noassert
    //     = ! Class.forName("[L<outermostClass>;").getComponentType()
    //     .desiredAssertionStatus();
    //
    //  ldc              "L[<outermostClass>;"
    //  invokevirtual    java/lang/Class.forName(Ljava/lang/String;)java/lang/Class
    //  invokevirtual    java/lang/Class.getComponentType()Ljava/lang/Class;
    //  invokevirtual    java/lang/Class.desiredAssertionStatus()Z
    //  iconst_1
    //  ixor             result ^ true <=> !result
    //  putstatic        <thisClass>.$noassert
    //
    assert(! control.option.noassert &&
           control.option.target >= JopaOption::SDK1_4);
    tsym = tsym -> GetArrayType(control.system_semantic, 1);
    LoadLiteral(tsym -> FindOrInsertClassLiteralName(control),
                control.String());
    PutOp(OP_INVOKESTATIC);
    PutU2(RegisterLibraryMethodref(control.Class_forNameMethod()));
    PutOp(OP_INVOKEVIRTUAL);
    ChangeStack(1); // for returned value
    PutU2(RegisterLibraryMethodref(control.Class_getComponentTypeMethod()));
    PutOp(OP_INVOKEVIRTUAL);
    ChangeStack(1); // for returned value
    PutU2(RegisterLibraryMethodref(control.Class_desiredAssertionStatusMethod()));
    PutOp(OP_ICONST_1);
    PutOp(OP_IXOR);
    PutOp(OP_PUTSTATIC);
    PutU2(RegisterFieldref(vsym));
}


int ByteCode::EmitName(AstName* expression, bool need_value)
{
    if (expression -> symbol -> TypeCast())
        return 0;
    VariableSymbol* var = expression -> symbol -> VariableCast();
    return LoadVariable((expression -> resolution_opt ? ACCESSED_VAR
                         : var -> owner -> MethodCast() ? LOCAL_VAR
                         : var -> ACC_STATIC() ? STATIC_VAR : FIELD_VAR),
                        expression, need_value);
}


//
// see also OP_MULTIANEWARRAY
//
int ByteCode::EmitArrayCreationExpression(AstArrayCreationExpression* expression,
                                          bool need_value)
{
    unsigned num_dims = expression -> NumDimExprs();

    if (expression -> Type() -> num_dimensions > 255)
        semantic.ReportSemError(SemanticError::ARRAY_OVERFLOW, expression);

    if (expression -> array_initializer_opt)
    {
        InitializeArray(expression -> Type(),
                        expression -> array_initializer_opt, need_value);
    }
    else
    {
        //
        // Need to push value of dimension(s) and create array. This can be
        // skipped if we don't need a value, but only if we know that all
        // dimensions are non-negative.
        //
        bool create_array = need_value;
        for (unsigned i = 0; ! create_array && i < num_dims; i++)
        {
            AstExpression* expr =
                StripNops(expression -> DimExpr(i) -> expression);
            if (expr -> IsConstant())
            {
                if (DYNAMIC_CAST<IntLiteralValue*> (expr -> value) ->
                    value < 0)
                {
                    create_array = true;
                }
            }
            else if (expr -> Type() != control.char_type)
                create_array = true;
        }
        for (unsigned j = 0; j < num_dims; j++)
            EmitExpression(expression -> DimExpr(j) -> expression,
                           create_array);
        if (create_array)
        {
            EmitNewArray(num_dims, expression -> Type());
            if (! need_value)
                PutOp(OP_POP);
        }
    }

    return need_value ? 1 : 0;
}


//
// ASSIGNMENT
//
int ByteCode::EmitAssignmentExpression(AstAssignmentExpression* assignment_expression,
                                       bool need_value)
{
    //
    // JLS2 added ability for parenthesized variable to remain a variable.
    //
    AstCastExpression* casted_left_hand_side =
        assignment_expression -> left_hand_side -> CastExpressionCast();
    AstExpression* left_hand_side
        = StripNops(casted_left_hand_side
                    ? casted_left_hand_side -> expression
                    : assignment_expression -> left_hand_side);

    TypeSymbol* left_type = left_hand_side -> Type();

    VariableCategory kind = GetVariableKind(assignment_expression);
    VariableSymbol* accessed_member = assignment_expression -> write_method
        ? (assignment_expression -> write_method -> accessed_member ->
           VariableCast())
        : (VariableSymbol*) NULL;

    if (assignment_expression -> SimpleAssignment())
    {
        switch (kind)
        {
        case ARRAY_VAR:
            // lhs must be array access
            EmitArrayAccessLhs(left_hand_side -> ArrayAccessCast());
            break;
        case FIELD_VAR:
            // load base for field access
            EmitFieldAccessLhsBase(left_hand_side);
            break;
        case STATIC_VAR:
            //
            // If the access is qualified by an arbitrary base
            // expression, evaluate it for side effects.
            //
            if (left_hand_side -> FieldAccessCast())
            {
                AstExpression* base =
                    ((AstFieldAccess*) left_hand_side) -> base;
                EmitExpression(base, false);
            }
            else if (left_hand_side -> NameCast())
            {
                AstName* base = ((AstName*) left_hand_side) -> base_opt;
                if (base)
                    EmitName(base, false);
            }
            break;
        case ACCESSED_VAR:
            // need to load address of object, obtained from resolution
            if (! accessed_member -> ACC_STATIC())
            {
                AstExpression* resolve = left_hand_side -> FieldAccessCast()
                    ? left_hand_side -> FieldAccessCast() -> resolution_opt
                    : left_hand_side -> NameCast() -> resolution_opt;
                assert(resolve);

                AstExpression* base =
                    resolve -> MethodInvocationCast() -> base_opt;
                assert(base);
                EmitExpression(base);
            }
            else if (left_hand_side -> FieldAccessCast())
                //
                // If the access is qualified by an arbitrary base
                // expression, evaluate it for side effects.
                //
                EmitExpression(((AstFieldAccess*) left_hand_side) -> base,
                               false);
            break;
        case LOCAL_VAR:
            break;
        default:
            assert(false && "bad kind in EmitAssignmentExpression");
        }

        EmitExpression(assignment_expression -> expression);
    }
    //
    // Here for compound assignment. Get the left operand, saving any
    // information necessary to update its value on the stack below the value.
    //
    else
    {
        switch (kind)
        {
        case ARRAY_VAR:
            // lhs must be array access
            EmitArrayAccessLhs(left_hand_side -> ArrayAccessCast());
            PutOp(OP_DUP2); // save base and index for later store

            //
            // load current value
            //
            LoadArrayElement(assignment_expression -> Type());
            break;
        case FIELD_VAR:
            EmitFieldAccessLhs(left_hand_side);
            break;
        case LOCAL_VAR:
            if (! casted_left_hand_side &&
                assignment_expression -> Type() == control.int_type &&
                assignment_expression -> expression -> IsConstant() &&
                ((assignment_expression -> Tag() ==
                  AstAssignmentExpression::PLUS_EQUAL) ||
                 (assignment_expression -> Tag() ==
                  AstAssignmentExpression::MINUS_EQUAL)))
            {
                IntLiteralValue* vp = DYNAMIC_CAST<IntLiteralValue*>
                    (assignment_expression -> expression -> value);
                int val = ((assignment_expression -> Tag() ==
                            AstAssignmentExpression::MINUS_EQUAL)
                           ? -(vp -> value) // we treat "a -= x" as "a += (-x)"
                           : vp -> value);
                if (val >= -32768 && val < 32768) // if value in range
                {
                    VariableSymbol* sym =
                        (VariableSymbol*) left_hand_side -> symbol;
                    PutOpIINC(sym -> LocalVariableIndex(), val);
                    LoadVariable(LOCAL_VAR, left_hand_side, need_value);
                    return GetTypeWords(assignment_expression -> Type());
                }
            }

            LoadVariable(kind, left_hand_side);
            break;
        case STATIC_VAR:
            LoadVariable(kind, left_hand_side);
            break;
        case ACCESSED_VAR:
            //
            // If we are accessing a static member, get value by invoking
            // appropriate resolution. Otherwise, in addition to getting
            // the value, we need to load address of the object,
            // obtained from the resolution, saving a copy on the stack.
            //
            if (accessed_member -> ACC_STATIC())
                EmitExpression(left_hand_side);
            else ResolveAccess(left_hand_side);
            break;
        default:
            assert(false && "bad kind in EmitAssignmentExpression");
        }

        //
        // Here for string concatenation.
        //
        if ((assignment_expression -> Tag() ==
             AstAssignmentExpression::PLUS_EQUAL) &&
            left_type == control.String())
        {
            PutOp(OP_NEW);
            PutU2(RegisterClass(control.option.target >= JopaOption::SDK1_5
                                ? control.StringBuilder()
                                : control.StringBuffer()));
            PutOp(OP_DUP_X1);
            PutOp(OP_INVOKESPECIAL);
            PutU2(RegisterLibraryMethodref
                  (control.option.target >= JopaOption::SDK1_5
                   ? control.StringBuilder_InitMethod()
                   : control.StringBuffer_InitMethod()));
            EmitStringAppendMethod(control.String());
            AppendString(assignment_expression -> expression, true);
            PutOp(OP_INVOKEVIRTUAL);
            PutU2(RegisterLibraryMethodref
                  (control.option.target >= JopaOption::SDK1_5
                   ? control.StringBuilder_toStringMethod()
                   : control.StringBuffer_toStringMethod()));
            ChangeStack(1); // account for return value
        }
        //
        // Here for operation other than string concatenation. Determine the
        // opcode to use.
        //
        else
        {
            Opcode opc = OP_NOP;

            TypeSymbol* op_type = (casted_left_hand_side
                                   ? casted_left_hand_side -> Type()
                                   : assignment_expression -> Type());

            if (control.IsSimpleIntegerValueType(op_type) ||
                op_type == control.boolean_type)
            {
                switch (assignment_expression -> Tag())
                {
                case AstAssignmentExpression::STAR_EQUAL:
                    opc = OP_IMUL;
                    break;
                case AstAssignmentExpression::SLASH_EQUAL:
                    opc = OP_IDIV;
                    break;
                case AstAssignmentExpression::MOD_EQUAL:
                    opc = OP_IREM;
                    break;
                case AstAssignmentExpression::PLUS_EQUAL:
                    opc = OP_IADD;
                    break;
                case AstAssignmentExpression::MINUS_EQUAL:
                    opc = OP_ISUB;
                    break;
                case AstAssignmentExpression::LEFT_SHIFT_EQUAL:
                    opc = OP_ISHL;
                    break;
                case AstAssignmentExpression::RIGHT_SHIFT_EQUAL:
                    opc = OP_ISHR;
                    break;
                case AstAssignmentExpression::UNSIGNED_RIGHT_SHIFT_EQUAL:
                    opc = OP_IUSHR;
                    break;
                case AstAssignmentExpression::AND_EQUAL:
                    opc = OP_IAND;
                    break;
                case AstAssignmentExpression::IOR_EQUAL:
                    opc = OP_IOR;
                    break;
                case AstAssignmentExpression::XOR_EQUAL:
                    opc = OP_IXOR;
                    break;
                default:
                    assert(false && "bad op_type in EmitAssignmentExpression");
                }
            }
            else if (op_type == control.long_type)
            {
                switch (assignment_expression -> Tag())
                {
                case AstAssignmentExpression::STAR_EQUAL:
                    opc = OP_LMUL;
                    break;
                case AstAssignmentExpression::SLASH_EQUAL:
                    opc = OP_LDIV;
                    break;
                case AstAssignmentExpression::MOD_EQUAL:
                    opc = OP_LREM;
                    break;
                case AstAssignmentExpression::PLUS_EQUAL:
                    opc = OP_LADD;
                    break;
                case AstAssignmentExpression::MINUS_EQUAL:
                    opc = OP_LSUB;
                    break;
                case AstAssignmentExpression::LEFT_SHIFT_EQUAL:
                    opc = OP_LSHL;
                    break;
                case AstAssignmentExpression::RIGHT_SHIFT_EQUAL:
                    opc = OP_LSHR;
                    break;
                case AstAssignmentExpression::UNSIGNED_RIGHT_SHIFT_EQUAL:
                    opc = OP_LUSHR;
                    break;
                case AstAssignmentExpression::AND_EQUAL:
                    opc = OP_LAND;
                    break;
                case AstAssignmentExpression::IOR_EQUAL:
                    opc = OP_LOR;
                    break;
                case AstAssignmentExpression::XOR_EQUAL:
                    opc = OP_LXOR;
                    break;
                default:
                    assert(false && "bad op_type in EmitAssignmentExpression");
                }
            }
            else if (op_type == control.float_type)
            {
                switch (assignment_expression -> Tag())
                {
                case AstAssignmentExpression::STAR_EQUAL:
                    opc = OP_FMUL;
                    break;
                case AstAssignmentExpression::SLASH_EQUAL:
                    opc = OP_FDIV;
                    break;
                case AstAssignmentExpression::MOD_EQUAL:
                    opc = OP_FREM;
                    break;
                case AstAssignmentExpression::PLUS_EQUAL:
                    opc = OP_FADD;
                    break;
                case AstAssignmentExpression::MINUS_EQUAL:
                    opc = OP_FSUB;
                    break;
                default:
                    assert(false && "bad op_type in EmitAssignmentExpression");
                }
            }
            else if (op_type == control.double_type)
            {
                switch (assignment_expression -> Tag())
                {
                case AstAssignmentExpression::STAR_EQUAL:
                    opc = OP_DMUL;
                    break;
                case AstAssignmentExpression::SLASH_EQUAL:
                    opc = OP_DDIV;
                    break;
                case AstAssignmentExpression::MOD_EQUAL:
                    opc = OP_DREM;
                    break;
                case AstAssignmentExpression::PLUS_EQUAL:
                    opc = OP_DADD;
                    break;
                case AstAssignmentExpression::MINUS_EQUAL:
                    opc = OP_DSUB;
                    break;
                default:
                    assert(false && "bad op_type in EmitAssignmentExpression");
                }
            }
            else
            {
                assert(false && "unrecognized op_type in EmitAssignmentExpression");
            }

            //
            // convert value to desired type if necessary
            //
            if (casted_left_hand_side)
                EmitCast(casted_left_hand_side -> Type(), left_type);

            EmitExpression(assignment_expression -> expression);

            PutOp(opc);

            if (casted_left_hand_side) // now cast result back to type of result
                EmitCast(left_type, casted_left_hand_side -> Type());
        }
    }

    //
    // Update left operand, saving value of right operand if it is needed.
    //
    switch (kind)
    {
    case ARRAY_VAR:
        if (need_value)
            PutOp(control.IsDoubleWordType(left_type) ? OP_DUP2_X2 : OP_DUP_X2);
        StoreArrayElement(assignment_expression -> Type());
        break;
    case FIELD_VAR:
        if (need_value)
            PutOp(control.IsDoubleWordType(left_type) ? OP_DUP2_X1 : OP_DUP_X1);
        StoreField(left_hand_side);
        break;
    case ACCESSED_VAR:
        {
            if (need_value)
            {
                if (accessed_member -> ACC_STATIC())
                    PutOp(control.IsDoubleWordType(left_type)
                          ? OP_DUP2 : OP_DUP);
                else PutOp(control.IsDoubleWordType(left_type)
                           ? OP_DUP2_X1 : OP_DUP_X1);
            }

            int stack_words = (GetTypeWords(left_type) +
                               (accessed_member -> ACC_STATIC() ? 0 : 1));
            PutOp(OP_INVOKESTATIC);
            CompleteCall(assignment_expression -> write_method, stack_words);
        }
        break;
    case LOCAL_VAR:
        //
        // Prior to JDK 1.5, VMs incorrectly complained if assigning an array
        // type into an element of a null expression (in other words, null
        // was not being treated as compatible with a multi-dimensional array
        // on the aastore opcode).  The workaround requires a checkcast any
        // time null might be assigned to a multi-dimensional local variable
        // or directly used as an array access base.
        //
        if (control.option.target < JopaOption::SDK1_5 &&
            IsMultiDimensionalArray(left_type) &&
            (StripNops(assignment_expression -> expression) -> Type() ==
             control.null_type))
        {
            assert(assignment_expression -> SimpleAssignment());
            PutOp(OP_CHECKCAST);
            PutU2(RegisterClass(left_type));
        }
        // fallthrough
    case STATIC_VAR:
        if (need_value)
            PutOp(control.IsDoubleWordType(left_type) ? OP_DUP2 : OP_DUP);
        StoreVariable(kind, left_hand_side);
        break;
    default:
        assert(false && "bad kind in EmitAssignmentExpression");
    }

    return GetTypeWords(assignment_expression -> Type());
}


//
// BINARY: Similar code patterns are used for the ordered comparisons. This
// method relies on the compiler having already inserted numeric promotion
// casts, so that the type of the left and right expressions match.
//
int ByteCode::EmitBinaryExpression(AstBinaryExpression* expression,
                                   bool need_value)
{
    TypeSymbol* type = expression -> Type();

    //
    // First, special case string concatenation.
    //
    if (type == control.String())
    {
        assert(expression -> Tag() == AstBinaryExpression::PLUS);
        ConcatenateString(expression, need_value);
        if (! need_value)
        {
            PutOp(OP_POP);
            return 0;
        }
        PutOp(OP_INVOKEVIRTUAL);
        PutU2(RegisterLibraryMethodref
              (control.option.target >= JopaOption::SDK1_5
               ? control.StringBuilder_toStringMethod()
               : control.StringBuffer_toStringMethod()));
        ChangeStack(1); // account for return value
        return 1;
    }

    //
    // Next, simplify if no result is needed. Be careful of side-effects with
    // binary / and % on integral 0, as well as evaluation order of && and ||.
    //
    if (! need_value)
    {
        if ((expression -> Tag() == AstBinaryExpression::SLASH ||
             expression -> Tag() == AstBinaryExpression::MOD) &&
            control.IsIntegral(type) &&
            (IsZero(expression -> right_expression) ||
             ! expression -> right_expression -> IsConstant()))
        {
            if (IsZero(expression -> right_expression))
            {
                //
                // Undo compiler-inserted numeric promotion.
                //
                AstExpression* left_expr = expression -> left_expression;
                if (left_expr -> CastExpressionCast() &&
                    left_expr -> generated)
                {
                    left_expr = ((AstCastExpression*) left_expr) -> expression;
                }
                type = left_expr -> Type();
                EmitExpression(left_expr);
                PutOp(type == control.long_type ? OP_LCONST_0 : OP_ICONST_0);
            }
            else
            {
                EmitExpression(expression -> left_expression);
                EmitExpression(expression -> right_expression);
            }
            if (type == control.long_type)
            {
                PutOp(expression -> Tag() == AstBinaryExpression::SLASH
                      ? OP_LDIV : OP_LREM);
                PutOp(OP_POP2);
            }
            else
            {
                PutOp(expression -> Tag() == AstBinaryExpression::SLASH
                      ? OP_IDIV : OP_IREM);
                PutOp(OP_POP);
            }
        }
        else if (expression -> Tag() == AstBinaryExpression::OR_OR)
        {
            //
            // if (cond || true); => cond;
            // if (cond || false); => cond;
            //
            if (expression -> right_expression -> IsConstant())
            {
                EmitExpression(expression -> left_expression, false);
            }
            //
            // if (true || cond); => nop
            // if (a || b); => if (!a) b;
            //
            else if (! IsOne(expression -> left_expression))
            {
                Label label;
                EmitBranchIfExpression(expression -> left_expression, true,
                                       label);
                EmitExpression(expression -> right_expression, false);
                DefineLabel(label);
                CompleteLabel(label);
            }
        }
        else if (expression -> Tag() == AstBinaryExpression::AND_AND)
        {
            //
            // if (cond && true); => cond;
            // if (cond && false); => cond;
            //
            if (expression -> right_expression -> IsConstant())
            {
                EmitExpression(expression -> left_expression, false);
            }
            //
            // if (false && cond); => nop
            // if (a && b); => if (a) b;
            //
            else if (! IsZero(expression -> left_expression))
            {
                Label label;
                EmitBranchIfExpression(expression -> left_expression, false,
                                       label);
                EmitExpression(expression -> right_expression, false);
                DefineLabel(label);
                CompleteLabel(label);
            }
        }
        else
        {
            EmitExpression(expression -> left_expression, false);
            EmitExpression(expression -> right_expression, false);
        }
        return 0;
    }

    //
    // Next, try to simplify if one operand known to be zero or one.
    //
    if (IsZero(expression -> left_expression))
    {
        //
        // Undo compiler-inserted numeric promotion, as well as narrowing from
        // long to int in shifts, to avoid unnecessary type conversions.
        //
        AstExpression* right_expr = expression -> right_expression;
        if (right_expr -> CastExpressionCast() && right_expr -> generated)
            right_expr = ((AstCastExpression*) right_expr) -> expression;
        TypeSymbol* right_type = right_expr -> Type();

        switch (expression -> Tag())
        {
        case AstBinaryExpression::AND_AND:
            PutOp(OP_ICONST_0);
            return 1;
        case AstBinaryExpression::EQUAL_EQUAL:
            if (right_type != control.boolean_type)
                break;
            EmitExpression(right_expr);
            PutOp(OP_ICONST_1);
            PutOp(OP_IXOR);
            return 1;
        case AstBinaryExpression::NOT_EQUAL:
            if (right_type != control.boolean_type)
                break;
            // Fallthrough on boolean case!
        case AstBinaryExpression::PLUS:
        case AstBinaryExpression::IOR:
        case AstBinaryExpression::XOR:
        case AstBinaryExpression::OR_OR:
            //
            // Note that +0.0 + expr cannot be simplified if expr is floating
            // point, because of -0.0 rules.
            //
            if (control.IsFloatingPoint(right_type))
            {
                if (expression -> left_expression -> Type() ==
                    control.float_type)
                {
                    FloatLiteralValue* value = DYNAMIC_CAST<FloatLiteralValue*>
                        (expression -> left_expression -> value);
                    if (value -> value.IsPositiveZero())
                        break;
                }
                else if (expression -> left_expression -> Type() ==
                         control.double_type)
                {
                    DoubleLiteralValue* value = DYNAMIC_CAST<DoubleLiteralValue*>
                        (expression -> left_expression -> value);
                    if (value -> value.IsPositiveZero())
                        break;
                }
            }
            // Use promoted version, not the stripped right_expr.
            EmitExpression(expression -> right_expression);
            return GetTypeWords(type);
        case AstBinaryExpression::STAR:
        case AstBinaryExpression::AND:
        case AstBinaryExpression::LEFT_SHIFT:
        case AstBinaryExpression::RIGHT_SHIFT:
        case AstBinaryExpression::UNSIGNED_RIGHT_SHIFT:
            //
            // Floating point multiplication by 0 cannot be simplified, because
            // of NaN, infinity, and -0.0 rules. And in general, division
            // cannot be simplified because of divide by 0 for integers and
            // corner cases for floating point.
            //
            if (control.IsFloatingPoint(type))
                break;

            EmitExpression(right_expr, false);
            PutOp(type == control.long_type ? OP_LCONST_0 : OP_ICONST_0);
            return GetTypeWords(type);
        case AstBinaryExpression::MINUS:
            //
            // 0 - x is negation, but note that +0.0 - expr cannot be
            // simplified if expr is floating point, because of -0.0 rules.
            //
            if (control.IsFloatingPoint(right_type))
            {
                if (expression -> left_expression -> Type() ==
                    control.float_type)
                {
                    FloatLiteralValue* value = DYNAMIC_CAST<FloatLiteralValue*>
                        (expression -> left_expression -> value);
                    if (value -> value.IsPositiveZero())
                        break;
                }
                else if (expression -> left_expression -> Type() ==
                         control.double_type)
                {
                    DoubleLiteralValue* value = DYNAMIC_CAST<DoubleLiteralValue*>
                        (expression -> left_expression -> value);
                    if (value -> value.IsPositiveZero())
                        break;
                }
            }
            // Use promoted version, not the stripped right_expr.
            EmitExpression(expression -> right_expression);

            PutOp(control.IsSimpleIntegerValueType(type) ? OP_INEG
                  : type == control.long_type ? OP_LNEG
                  : type == control.float_type ? OP_FNEG
                  : OP_DNEG); // double_type
            return GetTypeWords(type);
        default:
            break;
        }
    }

    if (IsOne(expression -> left_expression))
    {
        if (expression -> Tag() == AstBinaryExpression::STAR)
        {
            EmitExpression(expression -> right_expression);
            return GetTypeWords(type);
        }
        if (expression -> left_expression -> Type() == control.boolean_type)
        {
            switch (expression -> Tag())
            {
            case AstBinaryExpression::EQUAL_EQUAL:
            case AstBinaryExpression::AND_AND:
            case AstBinaryExpression::AND:
                EmitExpression(expression -> right_expression);
                break;
            case AstBinaryExpression::IOR:
                EmitExpression(expression -> right_expression, false);
                // Fallthrough
            case AstBinaryExpression::OR_OR:
                PutOp(OP_ICONST_1);
                break;
            case AstBinaryExpression::NOT_EQUAL:
            case AstBinaryExpression::XOR:
                EmitExpression(expression -> right_expression);
                PutOp(OP_ICONST_1);
                PutOp(OP_IXOR);
                break;
            default:
                assert(false && "Invalid operator on boolean");
            }
            return 1;
        }
    }

    if (IsZero(expression -> right_expression))
    {
        //
        // Undo compiler-inserted numeric promotion to avoid unnecessary type
        // conversions.
        //
        AstExpression* left_expr = expression -> left_expression;
        if (left_expr -> CastExpressionCast() && left_expr -> generated)
            left_expr = ((AstCastExpression*) left_expr) -> expression;
        TypeSymbol* left_type = left_expr -> Type();

        switch (expression -> Tag())
        {
        case AstBinaryExpression::EQUAL_EQUAL:
            if (left_type != control.boolean_type)
                break;
            EmitExpression(left_expr);
            PutOp(OP_ICONST_1);
            PutOp(OP_IXOR);
            return 1;
        case AstBinaryExpression::NOT_EQUAL:
            if (left_type != control.boolean_type)
                break;
            // Fallthrough on boolean case!
        case AstBinaryExpression::PLUS:
        case AstBinaryExpression::MINUS:
        case AstBinaryExpression::IOR:
        case AstBinaryExpression::XOR:
        case AstBinaryExpression::OR_OR:
        case AstBinaryExpression::LEFT_SHIFT:
        case AstBinaryExpression::RIGHT_SHIFT:
        case AstBinaryExpression::UNSIGNED_RIGHT_SHIFT:
            //
            // Here for cases that simplify to the left operand. Note that
            // (expr + +0.0) and (expr - -0.0) cannot be simplified if expr
            // is floating point, because of -0.0 rules.
            //
            if (control.IsFloatingPoint(left_type))
            {
                if (expression -> right_expression -> Type() ==
                    control.float_type)
                {
                    FloatLiteralValue* value = DYNAMIC_CAST<FloatLiteralValue*>
                        (expression -> right_expression -> value);
                    if (value -> value.IsPositiveZero() ==
                        (expression -> Tag() == AstBinaryExpression::PLUS))
                        break;
                }
                else if (expression -> right_expression -> Type() ==
                         control.double_type)
                {
                    DoubleLiteralValue* value = DYNAMIC_CAST<DoubleLiteralValue*>
                        (expression -> right_expression -> value);
                    if (value -> value.IsPositiveZero() ==
                        (expression -> Tag() == AstBinaryExpression::PLUS))
                        break;
                }
            }
            // Use promoted version, not the stripped left_expr.
            EmitExpression(expression -> left_expression);
            return GetTypeWords(type);
        case AstBinaryExpression::STAR:
        case AstBinaryExpression::AND:
        case AstBinaryExpression::AND_AND:
            //
            // Floating point multiplication by 0 cannot be simplified, because
            // of NaN, infinity, and -0.0 rules. And in general, division
            // cannot be simplified because of divide by 0 for integers and
            // corner cases for floating point.
            //
            if (control.IsFloatingPoint(type))
                break;

            EmitExpression(left_expr, false);
            PutOp(type == control.long_type ? OP_LCONST_0 : OP_ICONST_0);
            return GetTypeWords(type);
        default:
            break;
        }
    }

    if (IsOne(expression -> right_expression))
    {
        if (expression -> Tag() == AstBinaryExpression::STAR ||
            expression -> Tag() == AstBinaryExpression::SLASH)
        {
            EmitExpression(expression -> left_expression);
            return GetTypeWords(type);
        }
        if (expression -> right_expression -> Type() == control.boolean_type)
        {
            switch (expression -> Tag())
            {
            case AstBinaryExpression::EQUAL_EQUAL:
            case AstBinaryExpression::AND_AND:
            case AstBinaryExpression::AND:
                EmitExpression(expression -> left_expression);
                break;
            case AstBinaryExpression::IOR:
            case AstBinaryExpression::OR_OR:
                EmitExpression(expression -> left_expression, false);
                PutOp(OP_ICONST_1);
                break;
            case AstBinaryExpression::NOT_EQUAL:
            case AstBinaryExpression::XOR:
                EmitExpression(expression -> left_expression);
                PutOp(OP_ICONST_1);
                PutOp(OP_IXOR);
                break;
            default:
                assert(false && "Invalid operator on boolean");
            }
            return 1;
        }
    }

    //
    // Next, simplify all remaining boolean result expressions.
    //
    if (expression -> left_expression -> Type() == control.boolean_type &&
        (expression -> Tag() == AstBinaryExpression::EQUAL_EQUAL ||
         expression -> Tag() == AstBinaryExpression::NOT_EQUAL))
    {
        EmitExpression(expression -> left_expression);
        EmitExpression(expression -> right_expression);
        PutOp(OP_IXOR);
        if (expression -> Tag() == AstBinaryExpression::EQUAL_EQUAL)
        {
            PutOp(OP_ICONST_1);
            PutOp(OP_IXOR);
        }
        return 1;
    }

    switch (expression -> Tag())
    {
    case AstBinaryExpression::OR_OR:
    case AstBinaryExpression::AND_AND:
    case AstBinaryExpression::LESS:
    case AstBinaryExpression::LESS_EQUAL:
    case AstBinaryExpression::GREATER:
    case AstBinaryExpression::GREATER_EQUAL:
    case AstBinaryExpression::EQUAL_EQUAL:
    case AstBinaryExpression::NOT_EQUAL:
        {
            // Assume false, and update if true.
            Label label;
            PutOp(OP_ICONST_0); // push false
            EmitBranchIfExpression(expression, false, label);
            PutOp(OP_POP); // pop the false
            PutOp(OP_ICONST_1); // push true
            DefineLabel(label);
            CompleteLabel(label);
        }
        return 1;
    default:
        break;
    }

    //
    // Finally, if we get here, the expression cannot be optimized.
    //
    EmitExpression(expression -> left_expression);
    EmitExpression(expression -> right_expression);

    bool integer_type = type == control.boolean_type ||
        control.IsSimpleIntegerValueType(type);
    switch (expression -> Tag())
    {
    case AstBinaryExpression::STAR:
        PutOp(integer_type ? OP_IMUL
              : type == control.long_type ? OP_LMUL
              : type == control.float_type ? OP_FMUL
              : OP_DMUL); // double_type
        break;
    case AstBinaryExpression::SLASH:
        PutOp(integer_type ? OP_IDIV
              : type == control.long_type ? OP_LDIV
              : type == control.float_type ? OP_FDIV
              : OP_DDIV); // double_type
        break;
    case AstBinaryExpression::MOD:
        PutOp(integer_type ? OP_IREM
              : type == control.long_type ? OP_LREM
              : type == control.float_type ? OP_FREM
              : OP_DREM); // double_type
        break;
    case AstBinaryExpression::PLUS:
        PutOp(integer_type ? OP_IADD
              : type == control.long_type ? OP_LADD
              : type == control.float_type ? OP_FADD
              : OP_DADD); // double_type
        break;
    case AstBinaryExpression::MINUS:
        PutOp(integer_type ? OP_ISUB
              : type == control.long_type ? OP_LSUB
              : type == control.float_type ? OP_FSUB
              : OP_DSUB); // double_type
        break;
    case AstBinaryExpression::LEFT_SHIFT:
        PutOp(integer_type ? OP_ISHL : OP_LSHL);
        break;
    case AstBinaryExpression::RIGHT_SHIFT:
        PutOp(integer_type ? OP_ISHR : OP_LSHR);
        break;
    case AstBinaryExpression::UNSIGNED_RIGHT_SHIFT:
        PutOp(integer_type ? OP_IUSHR : OP_LUSHR);
        break;
    case AstBinaryExpression::AND:
        PutOp(integer_type ? OP_IAND : OP_LAND);
        break;
    case AstBinaryExpression::XOR:
        PutOp(integer_type ? OP_IXOR : OP_LXOR);
        break;
    case AstBinaryExpression::IOR:
        PutOp(integer_type ? OP_IOR : OP_LOR);
        break;
    default:
        assert(false && "binary unknown tag");
    }

    return GetTypeWords(expression -> Type());
}


int ByteCode::EmitInstanceofExpression(AstInstanceofExpression* expr,
                                       bool need_value)
{
    TypeSymbol* left_type = expr -> expression -> Type();
    TypeSymbol* right_type = expr -> type -> symbol;
    if (right_type -> num_dimensions > 255)
        semantic.ReportSemError(SemanticError::ARRAY_OVERFLOW, expr -> type);
    if (left_type == control.null_type)
    {
        //
        // We know the result: false. But emit the left expression,
        // in case of side effects in (expr ? null : null).
        //
        EmitExpression(expr -> expression, false);
        if (need_value)
            PutOp(OP_ICONST_0);
    }
    else if (expr -> expression -> IsConstant() ||
             expr -> expression -> BinaryExpressionCast())
    {
        //
        // We know the result: true, since the string literals and string
        // concats are non-null and String is a final class.
        //
        assert(left_type == control.String());
        EmitExpression(expr -> expression, false);
        if (need_value)
            PutOp(OP_ICONST_1);
    }
    else if ((expr -> expression -> ThisExpressionCast() ||
              expr -> expression -> SuperExpressionCast() ||
              expr -> expression -> ClassLiteralCast() ||
              expr -> expression -> ClassCreationExpressionCast() ||
              expr -> expression -> ArrayCreationExpressionCast()) &&
             left_type -> IsSubtype(right_type))
    {
        //
        // We know the result: true, since the expression is non-null.
        //
        EmitExpression(expr -> expression, false);
        if (need_value)
            PutOp(OP_ICONST_1);
    }
    else
    {
        EmitExpression(expr -> expression, need_value);
        if (need_value)
        {
            PutOp(OP_INSTANCEOF);
            PutU2(RegisterClass(right_type));
        }
    }
    return need_value ? 1 : 0;
}


int ByteCode::EmitCastExpression(AstCastExpression* expression,
                                 bool need_value)
{
    TypeSymbol* dest_type = expression -> Type();
    TypeSymbol* source_type = expression -> expression -> Type();
    if (dest_type -> num_dimensions > 255 && expression -> type)
    {
        semantic.ReportSemError(SemanticError::ARRAY_OVERFLOW,
                                expression -> type);
    }

    //
    // Object downcasts must be emitted, in case of a ClassCastException.
    //
    EmitExpression(expression -> expression,
                   need_value || dest_type -> IsSubtype(source_type));

    if (need_value || dest_type -> IsSubtype(source_type))
    {
        EmitCast(dest_type, source_type);
        if (! need_value)
        {
            assert(source_type -> IsSubtype(control.Object()));
            PutOp(OP_POP);
        }
    }

    return need_value ? GetTypeWords(dest_type) : 0;
}


void ByteCode::EmitCast(TypeSymbol* dest_type, TypeSymbol* source_type)
{
    if (source_type -> IsSubtype(dest_type) ||
        source_type == control.null_type)
    {
        return; // done if nothing to do
    }

    // Java 5: Handle boxing conversion (primitive → wrapper)
    if (control.option.source >= JopaOption::SDK1_5 &&
        semantic.IsBoxingConversion(source_type, dest_type))
    {
        EmitBoxingConversion(source_type, dest_type);
        return;
    }

    // Java 5: Handle unboxing conversion (wrapper → primitive)
    if (control.option.source >= JopaOption::SDK1_5 &&
        semantic.IsUnboxingConversion(source_type, dest_type))
    {
        EmitUnboxingConversion(source_type, dest_type);
        return;
    }

    if (control.IsSimpleIntegerValueType(source_type))
    {
        if (dest_type == control.int_type ||
            (source_type == control.byte_type &&
             dest_type == control.short_type))
        {
            return; // no conversion needed
        }
        Opcode op_kind = (dest_type == control.long_type ? OP_I2L
                          : dest_type == control.float_type ? OP_I2F
                          : dest_type == control.double_type ? OP_I2D
                          : dest_type == control.char_type ? OP_I2C
                          : dest_type == control.byte_type ? OP_I2B
                          : OP_I2S); // short_type
        // If the type we wanted to cast to could not be matched then
        // the cast is invalid. For example, one might be trying
        // to cast an int to a Object.
        assert(op_kind != OP_I2S || dest_type == control.short_type);

        PutOp(op_kind);
    }
    else if (source_type == control.long_type)
    {
        Opcode op_kind = (dest_type == control.float_type ? OP_L2F
                          : dest_type == control.double_type ? OP_L2D
                          : OP_L2I);
        PutOp(op_kind);

        if (op_kind == OP_L2I && dest_type != control.int_type)
        {
            assert(control.IsSimpleIntegerValueType(dest_type) &&
                   "unsupported conversion");

            PutOp(dest_type == control.char_type ? OP_I2C
                  : dest_type == control.byte_type ? OP_I2B
                  : OP_I2S); // short_type
        }
    }
    else if (source_type == control.float_type)
    {
        Opcode op_kind = (dest_type == control.long_type ? OP_F2L
                          : dest_type == control.double_type ? OP_F2D
                          : OP_F2I);
        PutOp(op_kind);

        if (op_kind == OP_F2I && dest_type != control.int_type)
        {
            assert(control.IsSimpleIntegerValueType(dest_type) &&
                   "unsupported conversion");

            PutOp(dest_type == control.char_type ? OP_I2C
                  : dest_type == control.byte_type ? OP_I2B
                  : OP_I2S); // short_type
        }
    }
    else if (source_type == control.double_type)
    {
        Opcode op_kind = (dest_type == control.long_type ? OP_D2L
                          : dest_type == control.float_type ? OP_D2F
                          : OP_D2I);

        PutOp(op_kind);

        if (op_kind == OP_D2I && dest_type != control.int_type)
        {
            assert(control.IsSimpleIntegerValueType(dest_type) &&
                   "unsupported conversion");

            PutOp(dest_type == control.char_type ? OP_I2C
                  : dest_type == control.byte_type ? OP_I2B
                  : OP_I2S); // short_type
        }
    }
    else
    {
        PutOp(OP_CHECKCAST);
        PutU2(RegisterClass(dest_type));
    }
}

//
// Emits the required check for null in a qualified instance creation,
// super constructor call, or constant instance variable reference, if the
// base expression can possibly be null. It also emits the base expression.
// In the case of anonymous classes, we emit an alternate expression (the
// constructor parameter), after performing the null check on the qualifier
// of the anonymous class instance creation expression.
//
void ByteCode::EmitCheckForNull(AstExpression* expression, bool need_value)
{
    expression = StripNops(expression);

    if (expression -> Type() == control.null_type)
    {
        //
        // It's guaranteed to be null, so cause any side effects, then throw
        // the null already on the stack (which will make the VM correctly
        // create and throw a NullPointerException). Adjust the stack if
        // necessary, since the calling context does not realize that this
        // will always complete abruptly.
        //
        EmitExpression(expression, true);
        PutOp(OP_ATHROW);
        if (need_value)
            ChangeStack(1);
        return;
    }
    VariableSymbol* variable = expression -> symbol -> VariableCast();
    if (expression -> ClassCreationExpressionCast() ||
        expression -> ThisExpressionCast() ||
        expression -> SuperExpressionCast() ||
        expression -> ClassLiteralCast() ||
        (variable && variable -> ACC_SYNTHETIC() &&
         variable -> Identity() == control.this_name_symbol))
    {
        EmitExpression(expression, need_value);
        return;
    }
    //
    // We did not bother checking for other guaranteed non-null conditions:
    // IsConstant(), string concats, and ArrayCreationExpressionCast(), since
    // none of these can qualify a constructor invocation or a constant
    // instance field reference. If we get here, it is uncertain whether the
    // expression can be null, so check, using:
    //
    // ((Object) ref).getClass();
    //
    // This discarded instance method call will cause the necessary
    // NullPointerException if invoked on null; and since it is final in
    // Object, we can be certain it has no side-effects.
    //
    EmitExpression(expression, true);
    if (need_value)
        PutOp(OP_DUP);
    PutOp(OP_INVOKEVIRTUAL);
    ChangeStack(1); // for returned value
    PutU2(RegisterLibraryMethodref(control.Object_getClassMethod()));
    PutOp(OP_POP);
}

int ByteCode::EmitClassCreationExpression(AstClassCreationExpression* expr,
                                          bool need_value)
{
    if (expr -> resolution_opt)
        expr = expr -> resolution_opt;
    MethodSymbol* constructor = (MethodSymbol*) expr -> symbol;
    TypeSymbol* type = constructor -> containing_type;

    PutOp(OP_NEW);
    PutU2(RegisterClass(type));
    if (need_value) // save address of new object for constructor
        PutOp(OP_DUP);

    //
    // Pass enclosing instance along, then real arguments, then shadow
    // variables, and finally an extra null argument, as needed.
    //
    int stack_words = 0;
    unsigned i = 0;
    if (expr -> base_opt)
    {
        stack_words++;
        EmitCheckForNull(expr -> base_opt);
    }
    if (type -> Anonymous() && type -> super -> EnclosingInstance())
    {
        stack_words++;
        EmitCheckForNull(expr -> arguments -> Argument(i++));
    }
    for ( ; i < expr -> arguments -> NumArguments(); i++)
        stack_words += EmitExpression(expr -> arguments -> Argument(i));
    for (i = 0; i < expr -> arguments -> NumLocalArguments(); i++)
        stack_words +=
            EmitExpression(expr -> arguments -> LocalArgument(i));
    if (expr -> arguments -> NeedsExtraNullArgument())
    {
        PutOp(OP_ACONST_NULL);
        stack_words++;
    }

    PutOp(OP_INVOKESPECIAL);
    ChangeStack(-stack_words);
    PutU2(RegisterMethodref(type, constructor));
    return 1;
}


int ByteCode::EmitConditionalExpression(AstConditionalExpression* expression,
                                        bool need_value)
{
    //
    // Optimize (true ? a : b) to (a).
    // Optimize (false ? a : b) (b).
    //
    if (expression -> test_expression -> IsConstant())
        return EmitExpression((IsZero(expression -> test_expression)
                               ? expression -> false_expression
                               : expression -> true_expression),
                              need_value);
    if (expression -> Type() == control.null_type)
    {
        //
        // The null literal has no side effects, but null_expr might.
        // Optimize (cond ? null : null) to (cond, null).
        // Optimize (cond ? null_expr : null) to (cond && null_expr, null).
        // Optimize (cond ? null : null_expr) to (cond || null_expr, null).
        //
        if (expression -> false_expression -> NullLiteralCast())
        {
            if (expression -> true_expression -> NullLiteralCast())
                EmitExpression(expression -> test_expression, false);
            else
            {
                Label lab;
                EmitBranchIfExpression(expression -> test_expression, false,
                                       lab);
                EmitExpression(expression -> true_expression, false);
                DefineLabel(lab);
                CompleteLabel(lab);
            }
            if (need_value)
                PutOp(OP_ACONST_NULL);
            return need_value ? 1 : 0;
        }
        if (expression -> true_expression -> NullLiteralCast())
        {
            Label lab;
            EmitBranchIfExpression(expression -> test_expression, true, lab);
            EmitExpression(expression -> false_expression, false);
            DefineLabel(lab);
            CompleteLabel(lab);
            if (need_value)
                PutOp(OP_ACONST_NULL);
            return need_value ? 1 : 0;
        }
    }
    else if (expression -> true_expression -> IsConstant())
    {
        if (expression -> false_expression -> IsConstant())
        {
            if (! need_value)
                return EmitExpression(expression -> test_expression, false);
            if (expression -> true_expression -> value ==
                expression -> false_expression -> value)
            {
                //
                // Optimize (cond ? expr : expr) to (cond, expr).
                //
                EmitExpression(expression -> test_expression, false);
                return EmitExpression(expression -> true_expression);
            }
            if (control.IsSimpleIntegerValueType(expression -> Type()) ||
                expression -> Type() == control.boolean_type)
            {
                //
                // Optimize (expr ? 1 : 0) to (expr).
                // Optimize (expr ? value + 1 : value) to (expr + value).
                // Optimize (expr ? value - 1 : value) to (value - expr).
                //
                IntLiteralValue* left = DYNAMIC_CAST<IntLiteralValue*>
                    (expression -> true_expression -> value);
                IntLiteralValue* right = DYNAMIC_CAST<IntLiteralValue*>
                    (expression -> false_expression -> value);
                if (left -> value == 1 && right -> value == 0)
                    return EmitExpression(expression -> test_expression);
                if (left -> value == right -> value + 1)
                {
                    EmitExpression(expression -> test_expression);
                    EmitExpression(expression -> false_expression);
                    PutOp(OP_IADD);
                    return 1;
                }
                if (left -> value == right -> value - 1)
                {
                    EmitExpression(expression -> false_expression);
                    EmitExpression(expression -> test_expression);
                    PutOp(OP_ISUB);
                    return 1;
                }
            }
        }
        else if ((control.IsSimpleIntegerValueType(expression -> Type()) ||
                  expression -> Type() == control.boolean_type) &&
                 (IsOne(expression -> true_expression) ||
                  IsZero(expression -> true_expression)))
        {
            //
            // Optimize (cond ? 1 : b) to (cond || b)
            // Optimize (cond ? 0 : b) to (!cond && b)
            //
            Label label;
            if (need_value)
                PutOp(IsZero(expression -> true_expression)
                      ? OP_ICONST_0 : OP_ICONST_1);
            EmitBranchIfExpression(expression -> test_expression, true, label);
            if (need_value)
                PutOp(OP_POP);
            EmitExpression(expression -> false_expression, need_value);
            DefineLabel(label);
            CompleteLabel(label);
            return need_value ? 1 : 0;
        }
    }
    else if ((control.IsSimpleIntegerValueType(expression -> Type()) ||
              expression -> Type() == control.boolean_type) &&
             (IsOne(expression -> false_expression) ||
              IsZero(expression -> false_expression)))
    {
        //
        // Optimize (cond ? a : 0) to (cond && a)
        // Optimize (cond ? a : 1) to (!cond || a)
        //
        Label label;
        if (need_value)
            PutOp(IsZero(expression -> false_expression)
                  ? OP_ICONST_0 : OP_ICONST_1);
        EmitBranchIfExpression(expression -> test_expression, false, label);
        if (need_value)
            PutOp(OP_POP);
        EmitExpression(expression -> true_expression, need_value);
        DefineLabel(label);
        CompleteLabel(label);
        return need_value ? 1 : 0;
    }
    Label lab1,
        lab2;
    EmitBranchIfExpression(expression -> test_expression, false, lab1);
    EmitExpression(expression -> true_expression, need_value);
    EmitBranch(OP_GOTO, lab2);
    if (need_value) // restore the stack size
        ChangeStack(- GetTypeWords(expression -> Type()));
    DefineLabel(lab1);
    EmitExpression(expression -> false_expression, need_value);
    DefineLabel(lab2);
    CompleteLabel(lab2);
    CompleteLabel(lab1);
    return GetTypeWords(expression -> true_expression -> Type());
}


int ByteCode::EmitFieldAccess(AstFieldAccess* expression, bool need_value)
{
    if (expression -> resolution_opt)
        return LoadVariable(ACCESSED_VAR, expression, need_value);
    VariableSymbol* sym = expression -> symbol -> VariableCast();
    assert(sym);
    return LoadVariable(sym -> ACC_STATIC() ? STATIC_VAR : FIELD_VAR,
                        expression, need_value);
}


int ByteCode::EmitMethodInvocation(AstMethodInvocation* expression,
                                   bool need_value)
{
    //
    // If the method call was resolved into a call to another method, use the
    // resolution expression.
    //
    AstMethodInvocation* method_call = expression -> resolution_opt
        ? expression -> resolution_opt -> MethodInvocationCast() : expression;
    assert(method_call);
    MethodSymbol* msym = (MethodSymbol*) method_call -> symbol;
    AstExpression* base = method_call -> base_opt;
    bool is_super = false; // set if super call

    if (msym -> ACC_STATIC())
    {
        //
        // If the access is qualified by an arbitrary base
        // expression, evaluate it for side effects.
        // Notice that accessor methods, which are always static, might
        // access an instance method, in which case the base expression
        // will already be evaluated as the first parameter.
        //
        if (base && (! msym -> accessed_member ||
                      msym -> AccessesStaticMember()))
        {
            EmitExpression(base, false);
        }
    }
    else
    {
        if (base)
        {
            //
            // Note that field will be marked IsSuperAccess only in synthetic
            // accessor methods.  Code that calls Foo.super.bar() in a nested
            // class creates an accessor method:
            // Foo.access$<num>(Foo $1) { $1.bar(); }
            // but must use invokespecial instead of the regular invokevirtual.
            //
            is_super = base -> SuperExpressionCast() != NULL;
            EmitExpression(base);
        }
        else PutOp(OP_ALOAD_0);
    }

    int stack_words = 0; // words on stack needed for arguments
    unsigned num_args = method_call -> arguments -> NumArguments();
    unsigned num_formals = msym -> NumFormalParameters();

    if (msym -> ACC_VARARGS())
    {
        // Varargs method - emit fixed args, then create array for varargs
        unsigned num_fixed = num_formals > 0 ? num_formals - 1 : 0;

        // Emit fixed arguments
        for (unsigned i = 0; i < num_fixed && i < num_args; i++)
            stack_words += EmitExpression(method_call -> arguments -> Argument(i));

        // Emit or create the varargs array
        if (num_args >= num_formals)
        {
            // Array was already created by MethodInvocationConversion
            stack_words += EmitExpression(method_call -> arguments -> Argument(num_fixed));
        }
        else
        {
            // Create empty array for missing varargs
            TypeSymbol* varargs_type = msym -> FormalParameter(num_formals - 1) -> Type();
            assert(varargs_type -> IsArray());
            TypeSymbol* component_type = varargs_type -> ArraySubtype();
            assert(component_type && "component_type is null for varargs array");

            // Generate: anewarray <component_type> with count 0
            PutOp(OP_ICONST_0);
            PutOp(OP_ANEWARRAY);
            PutU2(RegisterClass(component_type));
            ChangeStack(0); // iconst_0 (+1), anewarray uses it and produces array (+0 net)
            stack_words++;
        }
    }
    else
    {
        // Non-varargs method - emit all arguments
        for (unsigned i = 0; i < num_args; i++)
            stack_words += EmitExpression(method_call -> arguments -> Argument(i));
    }

    TypeSymbol* type = MethodTypeResolution(method_call -> base_opt, msym);
    PutOp(msym -> ACC_STATIC() ? OP_INVOKESTATIC
          : (is_super || msym -> ACC_PRIVATE()) ? OP_INVOKESPECIAL
          : type -> ACC_INTERFACE() ? OP_INVOKEINTERFACE
          : OP_INVOKEVIRTUAL);
    return CompleteCall(msym, stack_words, need_value, type);
}


int ByteCode::CompleteCall(MethodSymbol* msym, int stack_words,
                           bool need_value, TypeSymbol* base_type)
{
    ChangeStack(- stack_words);
    TypeSymbol* type = (base_type ? base_type : msym -> containing_type);
    PutU2(RegisterMethodref(type, msym));
    // invokeinterface requires extra bytes (count and zero)
    // but invokestatic doesn't, even for interface methods (Java 8+)
    if (type -> ACC_INTERFACE() && ! msym -> ACC_STATIC())
    {
        PutU1(stack_words + 1);
        PutU1(0);
    }

    //
    // Must account for value returned by method.
    //
    if (msym -> Type() == control.void_type)
        return 0;
    bool wide = control.IsDoubleWordType(msym -> Type());
    ChangeStack(wide ? 2 : 1);
    if (! need_value)
    {
        PutOp(wide ? OP_POP2 : OP_POP);
        return 0;
    }
    return wide ? 2 : 1;
}


//
// Called when expression has been parenthesized; remove parentheses and
// widening casts to expose true structure.
//
AstExpression* ByteCode::StripNops(AstExpression* expr)
{
    while (! expr -> IsConstant())
    {
        if (expr -> ParenthesizedExpressionCast())
            expr = ((AstParenthesizedExpression*) expr) -> expression;
        else if (expr -> CastExpressionCast())
        {
            AstCastExpression* cast_expr = (AstCastExpression*) expr;
            TypeSymbol* cast_type = expr -> Type();
            AstExpression* sub_expr = StripNops(cast_expr -> expression);
            TypeSymbol* sub_type = sub_expr -> Type();
            if (sub_type -> IsSubtype(cast_type) ||
                (sub_type == control.byte_type &&
                 (cast_type == control.short_type ||
                  cast_type == control.int_type)) ||
                ((sub_type == control.short_type ||
                  sub_type == control.char_type) &&
                 cast_type == control.int_type) ||
                (sub_type == control.null_type &&
                 cast_type -> num_dimensions <= 255))
            {
                return sub_expr;
            }
            else return expr;
        }
        else return expr;
    }

    return expr;
}


bool ByteCode::IsNop(AstBlock* block)
{
    for (int i = static_cast<int>(block -> NumStatements()) - 1; i >= 0; i--)
    {
        Ast* statement = block -> Statement(i);
        if (statement -> EmptyStatementCast() ||
            statement -> LocalClassStatementCast() ||
            (statement -> BlockCast() && IsNop((AstBlock*) statement)))
            continue;
        if (statement -> kind == Ast::IF)
        {
            AstIfStatement* ifstat = (AstIfStatement*) statement;
            if ((IsOne(ifstat -> expression) &&
                 IsNop(ifstat -> true_statement)) ||
                (IsZero(ifstat -> expression) &&
                 (! ifstat -> false_statement_opt ||
                  IsNop(ifstat -> false_statement_opt))))
            {
                continue;
            }
        }
        //
        // TODO: Is it worth adding more checks for bypassed code?
        //
        return false;
    }
    return true;
}


void ByteCode::EmitNewArray(unsigned num_dims, const TypeSymbol* type)
{
    assert(num_dims);
    if (num_dims == 1)
    {
        TypeSymbol* element_type = type -> ArraySubtype();

        if (control.IsPrimitive(element_type))
        {
            PutOp(OP_NEWARRAY);
            PutU1(element_type == control.boolean_type ? 4
                  : element_type == control.char_type ? 5
                  : element_type == control.float_type ? 6
                  : element_type == control.double_type ? 7
                  : element_type == control.byte_type ? 8
                  : element_type == control.short_type ? 9
                  : element_type == control.int_type ? 10
                  : 11); // control.long_type
        }
        else // must be reference type
        {
            PutOp(OP_ANEWARRAY);
            PutU2(RegisterClass(element_type));
        }
    }
    else
    {
        PutOp(OP_MULTIANEWARRAY);
        PutU2(RegisterClass(type));
        PutU1(num_dims); // load dims count
        ChangeStack(1 - static_cast<int>(num_dims));
    }
}


//
// Initial part of array access: ready to either load or store after this.
//
void ByteCode::EmitArrayAccessLhs(AstArrayAccess* expression)
{
    TypeSymbol* base_type = expression -> base -> Type();
    AstExpression* base = StripNops(expression -> base);
    EmitExpression(base);
    if (control.option.target < JopaOption::SDK1_5 &&
        IsMultiDimensionalArray(base_type) &&
        base -> Type() == control.null_type)
    {
        //
        // Prior to JDK 1.5, VMs incorrectly complained if assigning an array
        // type into an element of a null expression (in other words, null
        // was not being treated as compatible with a multi-dimensional array
        // on the aastore opcode).  The workaround requires a checkcast any
        // time null might be assigned to a multi-dimensional local variable
        // or directly used as an array access base.
        //
        PutOp(OP_CHECKCAST);
        PutU2(RegisterClass(base_type));
    }
    EmitExpression(expression -> expression);
}

//
// POST_UNARY
//
int ByteCode::EmitPostUnaryExpression(AstPostUnaryExpression* expression,
                                      bool need_value)
{
    VariableCategory kind = GetVariableKind(expression);

    switch (kind)
    {
    case LOCAL_VAR:
    case STATIC_VAR:
        EmitPostUnaryExpressionSimple(kind, expression, need_value);
        break;
    case ARRAY_VAR:
        EmitPostUnaryExpressionArray(expression, need_value);
        break;
    case FIELD_VAR:
        EmitPostUnaryExpressionField(kind, expression, need_value);
        break;
    case ACCESSED_VAR:
        {
            VariableSymbol* accessed_member =
                expression -> write_method -> accessed_member -> VariableCast();
            if (accessed_member -> ACC_STATIC())
                EmitPostUnaryExpressionSimple(kind, expression, need_value);
            else EmitPostUnaryExpressionField(kind, expression, need_value);
        }
        break;
    default:
        assert(false && "unknown lhs kind for assignment");
    }

    return GetTypeWords(expression -> Type());
}


//
// AstExpression* expression;
// POST_UNARY on instance variable
// load value of field, duplicate, do increment or decrement, then store
// back, leaving original value on top of stack.
//
void ByteCode::EmitPostUnaryExpressionField(VariableCategory kind,
                                            AstPostUnaryExpression* expression,
                                            bool need_value)
{
    if (kind == ACCESSED_VAR)
        ResolveAccess(expression -> expression); // get address and value
    else EmitFieldAccessLhs(expression -> expression);

    TypeSymbol* expression_type = expression -> Type();
    if (need_value)
        PutOp(control.IsDoubleWordType(expression_type)
              ? OP_DUP2_X1 : OP_DUP_X1);

    if (control.IsSimpleIntegerValueType(expression_type))
    {
        PutOp(OP_ICONST_1);
        PutOp(expression -> Tag() == AstPostUnaryExpression::PLUSPLUS
              ? OP_IADD : OP_ISUB);
        EmitCast(expression_type, control.int_type);
    }
    else if (expression_type == control.long_type)
    {
        PutOp(OP_LCONST_1);
        PutOp(expression -> Tag() == AstPostUnaryExpression::PLUSPLUS
              ? OP_LADD : OP_LSUB);
    }
    else if (expression_type == control.float_type)
    {
        PutOp(OP_FCONST_1);
        PutOp(expression -> Tag() == AstPostUnaryExpression::PLUSPLUS
              ? OP_FADD : OP_FSUB);
    }
    else if (expression_type == control.double_type)
    {
        PutOp(OP_DCONST_1); // load 1.0
        PutOp(expression -> Tag() == AstPostUnaryExpression::PLUSPLUS
              ? OP_DADD : OP_DSUB);
    }

    if (kind == ACCESSED_VAR)
    {
        int stack_words = GetTypeWords(expression_type) + 1;
        PutOp(OP_INVOKESTATIC);
        CompleteCall(expression -> write_method, stack_words);
    }
    else // assert(kind == FIELD_VAR)
    {
        PutOp(OP_PUTFIELD);
        if (control.IsDoubleWordType(expression_type))
            ChangeStack(-1);

        VariableSymbol* sym = (VariableSymbol*) expression -> symbol;
        PutU2(RegisterFieldref(VariableTypeResolution(expression ->
                                                      expression, sym), sym));
    }
}


//
// AstExpression* expression;
// POST_UNARY on local variable
// load value of variable, duplicate, do increment or decrement, then store
// back, leaving original value on top of stack.
//
void ByteCode::EmitPostUnaryExpressionSimple(VariableCategory kind,
                                             AstPostUnaryExpression* expression,
                                             bool need_value)
{
    TypeSymbol* expression_type = expression -> Type();
    if (kind == LOCAL_VAR && expression_type == control.int_type)
    {
        // can we use IINC ??
        LoadVariable(kind, StripNops(expression -> expression), need_value);
        PutOpIINC(expression -> symbol -> VariableCast() -> LocalVariableIndex(),
                  expression -> Tag() == AstPostUnaryExpression::PLUSPLUS ? 1 : -1);
        return;
    }

    // this will also load value needing resolution
    LoadVariable(kind, StripNops(expression -> expression));

    if (need_value)
        PutOp(control.IsDoubleWordType(expression_type) ? OP_DUP2 : OP_DUP);

    if (control.IsSimpleIntegerValueType(expression_type))
    {
        PutOp(OP_ICONST_1);
        PutOp(expression -> Tag() == AstPostUnaryExpression::PLUSPLUS
              ? OP_IADD : OP_ISUB);
        EmitCast(expression_type, control.int_type);
    }
    else if (expression_type == control.long_type)
    {
        PutOp(OP_LCONST_1);
        PutOp(expression -> Tag() == AstPostUnaryExpression::PLUSPLUS
              ? OP_LADD : OP_LSUB);
    }
    else if (expression_type == control.float_type)
    {
        PutOp(OP_FCONST_1);
        PutOp(expression -> Tag() == AstPostUnaryExpression::PLUSPLUS
              ? OP_FADD : OP_FSUB);
    }
    else if (expression_type == control.double_type)
    {
        PutOp(OP_DCONST_1); // load 1.0
        PutOp(expression -> Tag() == AstPostUnaryExpression::PLUSPLUS
              ? OP_DADD : OP_DSUB);
    }

    if (kind == ACCESSED_VAR)
    {
         int stack_words = GetTypeWords(expression_type);
         PutOp(OP_INVOKESTATIC);
         CompleteCall(expression -> write_method, stack_words);
    }
    else StoreVariable(kind, expression -> expression);
}


//
// Post Unary for which operand is array element
// assignment for which lhs is array element
//    AstExpression* expression;
//
void ByteCode::EmitPostUnaryExpressionArray(AstPostUnaryExpression* expression,
                                            bool need_value)
{
    //
    // JLS2 added ability for parenthesized variable to remain a variable.
    //
    EmitArrayAccessLhs((AstArrayAccess*) StripNops(expression -> expression));
    // lhs must be array access
    PutOp(OP_DUP2); // save array base and index for later store

    TypeSymbol* expression_type = expression -> Type();
    if (expression_type == control.int_type)
    {
         PutOp(OP_IALOAD);
         if (need_value) // save value below saved array base and index
             PutOp(OP_DUP_X2);
         PutOp(OP_ICONST_1);
         PutOp(expression -> Tag() == AstPostUnaryExpression::PLUSPLUS
               ? OP_IADD : OP_ISUB);
         PutOp(OP_IASTORE);
    }
    else if (expression_type == control.byte_type )
    {
         PutOp(OP_BALOAD);
         if (need_value) // save value below saved array base and index
             PutOp(OP_DUP_X2);
         PutOp(OP_ICONST_1);
         PutOp(expression -> Tag() == AstPostUnaryExpression::PLUSPLUS
               ? OP_IADD : OP_ISUB);
         PutOp(OP_I2B);
         PutOp(OP_BASTORE);
    }
    else if (expression_type == control.char_type )
    {
         PutOp(OP_CALOAD);
         if (need_value) // save value below saved array base and index
             PutOp(OP_DUP_X2);
         PutOp(OP_ICONST_1);
         PutOp(expression -> Tag() == AstPostUnaryExpression::PLUSPLUS
               ? OP_IADD : OP_ISUB);
         PutOp(OP_I2C);
         PutOp(OP_CASTORE);
    }
    else if (expression_type == control.short_type)
    {
         PutOp(OP_SALOAD);
         if (need_value) // save value below saved array base and index
             PutOp(OP_DUP_X2);
         PutOp(OP_ICONST_1);
         PutOp(expression -> Tag() == AstPostUnaryExpression::PLUSPLUS
               ? OP_IADD : OP_ISUB);
         PutOp(OP_I2S);
         PutOp(OP_SASTORE);
    }
    else if (expression_type == control.long_type)
    {
         PutOp(OP_LALOAD);
         if (need_value) // save value below saved array base and index
             PutOp(OP_DUP2_X2);
         PutOp(OP_LCONST_1);
         PutOp(expression -> Tag() == AstPostUnaryExpression::PLUSPLUS
               ? OP_LADD : OP_LSUB);
         PutOp(OP_LASTORE);
    }
    else if (expression_type == control.float_type)
    {
         PutOp(OP_FALOAD);
         if (need_value) // save value below saved array base and index
             PutOp(OP_DUP_X2);
         PutOp(OP_FCONST_1);
         PutOp(expression -> Tag() == AstPostUnaryExpression::PLUSPLUS
               ? OP_FADD : OP_FSUB);
         PutOp(OP_FASTORE);
    }
    else if (expression_type == control.double_type)
    {
         PutOp(OP_DALOAD);
         if (need_value) // save value below saved array base and index
             PutOp(OP_DUP2_X2);
         PutOp(OP_DCONST_1);
         PutOp(expression -> Tag() == AstPostUnaryExpression::PLUSPLUS
               ? OP_DADD : OP_DSUB);
         PutOp(OP_DASTORE);
    }
    else assert(false && "unsupported postunary type");
}


//
// PRE_UNARY
//
int ByteCode::EmitPreUnaryExpression(AstPreUnaryExpression* expression,
                                     bool need_value)
{
    TypeSymbol* type = expression -> Type();
    if (expression -> Tag() == AstPreUnaryExpression::PLUSPLUS ||
        expression -> Tag() == AstPreUnaryExpression::MINUSMINUS)
    {
        EmitPreUnaryIncrementExpression(expression, need_value);
    }
    else // here for ordinary unary operator without side effects.
    {
        EmitExpression(expression -> expression, need_value);
        if (! need_value)
            return 0;
        switch (expression -> Tag())
        {
        case AstPreUnaryExpression::PLUS:
            // Nothing else to do.
            break;
        case AstPreUnaryExpression::MINUS:
            assert(control.IsNumeric(type) && "unary minus on bad type");
            PutOp(control.IsSimpleIntegerValueType(type) ? OP_INEG
                  : type == control.long_type ? OP_LNEG
                  : type == control.float_type ? OP_FNEG
                  : OP_DNEG); // double_type
            break;
        case AstPreUnaryExpression::TWIDDLE:
            if (control.IsSimpleIntegerValueType(type))
            {
                PutOp(OP_ICONST_M1); // -1
                PutOp(OP_IXOR);      // exclusive or to get result
            }
            else if (type == control.long_type)
            {
                PutOp(OP_LCONST_1); // make -1
                PutOp(OP_LNEG);
                PutOp(OP_LXOR);     // exclusive or to get result
            }
            else assert(false && "unary ~ on unsupported type");
            break;
        case AstPreUnaryExpression::NOT:
            assert(type == control.boolean_type);
            PutOp(OP_ICONST_1);
            PutOp(OP_IXOR); // !(e) <=> (e)^true
            break;
        default:
            assert(false && "unknown preunary tag");
        }
    }
    return GetTypeWords(type);
}


//
// PRE_UNARY with side effects (++X or --X)
//
void ByteCode::EmitPreUnaryIncrementExpression(AstPreUnaryExpression* expression,
                                               bool need_value)
{
    VariableCategory kind = GetVariableKind(expression);

    switch (kind)
    {
    case LOCAL_VAR:
    case STATIC_VAR:
        EmitPreUnaryIncrementExpressionSimple(kind, expression, need_value);
        break;
    case ARRAY_VAR:
        EmitPreUnaryIncrementExpressionArray(expression, need_value);
        break;
    case FIELD_VAR:
        EmitPreUnaryIncrementExpressionField(kind, expression, need_value);
        break;
    case ACCESSED_VAR:
        {
            VariableSymbol* accessed_member =
                expression -> write_method -> accessed_member -> VariableCast();
            if (accessed_member -> ACC_STATIC())
                EmitPreUnaryIncrementExpressionSimple(kind, expression,
                                                      need_value);
            else EmitPreUnaryIncrementExpressionField(kind, expression,
                                                      need_value);
        }
        break;
    default:
        assert(false && "unknown lhs kind for assignment");
    }
}


//
// AstExpression* expression;
// PRE_UNARY on name
// load value of variable, do increment or decrement, duplicate, then store
// back, leaving new value on top of stack.
//
void ByteCode::EmitPreUnaryIncrementExpressionSimple(VariableCategory kind,
                                                     AstPreUnaryExpression* expression,
                                                     bool need_value)
{
    TypeSymbol* type = expression -> Type();
    if (kind == LOCAL_VAR && type == control.int_type)
    {
        PutOpIINC(expression -> symbol -> VariableCast() -> LocalVariableIndex(),
                  expression -> Tag() == AstPreUnaryExpression::PLUSPLUS ? 1 : -1);
        LoadVariable(kind, StripNops(expression -> expression), need_value);
        return;
    }

    // will also load value if resolution needed
    LoadVariable(kind, StripNops(expression -> expression));

    if (control.IsSimpleIntegerValueType(type))
    {
        PutOp(OP_ICONST_1);
        PutOp(expression -> Tag() == AstPreUnaryExpression::PLUSPLUS
              ? OP_IADD : OP_ISUB);
        EmitCast(type, control.int_type);
        if (need_value)
            PutOp(OP_DUP);
    }
    else if (type == control.long_type)
    {
        PutOp(OP_LCONST_1);
        PutOp(expression -> Tag() == AstPreUnaryExpression::PLUSPLUS
              ? OP_LADD : OP_LSUB);
        if (need_value)
            PutOp(OP_DUP2);
    }
    else if (type == control.float_type)
    {
        PutOp(OP_FCONST_1);
        PutOp(expression -> Tag() == AstPreUnaryExpression::PLUSPLUS
              ? OP_FADD : OP_FSUB);
        if (need_value)
            PutOp(OP_DUP);
    }
    else if (type == control.double_type)
    {
        PutOp(OP_DCONST_1); // load 1.0
        PutOp(expression -> Tag() == AstPreUnaryExpression::PLUSPLUS
              ? OP_DADD : OP_DSUB);
        if (need_value)
            PutOp(OP_DUP2);
    }

    if (kind == ACCESSED_VAR)
    {
        int stack_words = GetTypeWords(type);
        PutOp(OP_INVOKESTATIC);
        CompleteCall(expression -> write_method, stack_words);
    }
    else StoreVariable(kind, expression -> expression);
}


//
// Post Unary for which operand is array element
// assignment for which lhs is array element
//    AstExpression* expression;
//
void ByteCode::EmitPreUnaryIncrementExpressionArray(AstPreUnaryExpression* expression,
                                                    bool need_value)
{
    //
    // JLS2 added ability for parenthesized variable to remain a variable.
    //
    // lhs must be array access
    EmitArrayAccessLhs((AstArrayAccess*) StripNops(expression -> expression));

    PutOp(OP_DUP2); // save array base and index for later store

    TypeSymbol* type = expression -> Type();
    if (type == control.int_type)
    {
         PutOp(OP_IALOAD);
         PutOp(OP_ICONST_1);
         PutOp(expression -> Tag() == AstPreUnaryExpression::PLUSPLUS
               ? OP_IADD : OP_ISUB);
         if (need_value)
             PutOp(OP_DUP_X2);
         PutOp(OP_IASTORE);
    }
    else if (type == control.byte_type)
    {
         PutOp(OP_BALOAD);
         PutOp(OP_ICONST_1);
         PutOp(expression -> Tag() == AstPreUnaryExpression::PLUSPLUS
               ? OP_IADD : OP_ISUB);
         PutOp(OP_I2B);
         if (need_value)
             PutOp(OP_DUP_X2);
         PutOp(OP_BASTORE);
    }
    else if (type == control.char_type)
    {
         PutOp(OP_CALOAD);
         PutOp(OP_ICONST_1);
         PutOp(expression -> Tag() == AstPreUnaryExpression::PLUSPLUS
               ? OP_IADD : OP_ISUB);
         PutOp(OP_I2C);
         if (need_value)
             PutOp(OP_DUP_X2);
         PutOp(OP_CASTORE);
    }
    else if (type == control.short_type)
    {
         PutOp(OP_SALOAD);
         PutOp(OP_ICONST_1);
         PutOp(expression -> Tag() == AstPreUnaryExpression::PLUSPLUS
               ? OP_IADD : OP_ISUB);
         PutOp(OP_I2S);
         if (need_value)
             PutOp(OP_DUP_X2);
         PutOp(OP_SASTORE);
    }
    else if (type == control.long_type)
    {
         PutOp(OP_LALOAD);
         PutOp(OP_LCONST_1);
         PutOp(expression -> Tag() == AstPreUnaryExpression::PLUSPLUS
               ? OP_LADD : OP_LSUB);
         if (need_value)
             PutOp(OP_DUP2_X2);
         PutOp(OP_LASTORE);
    }
    else if (type == control.float_type)
    {
         PutOp(OP_FALOAD);
         PutOp(OP_FCONST_1);
         PutOp(expression -> Tag() == AstPreUnaryExpression::PLUSPLUS
               ? OP_FADD : OP_FSUB);
         if (need_value)
             PutOp(OP_DUP_X2);
         PutOp(OP_FASTORE);
    }
    else if (type == control.double_type)
    {
         PutOp(OP_DALOAD);
         PutOp(OP_DCONST_1);
         PutOp(expression -> Tag() == AstPreUnaryExpression::PLUSPLUS
               ? OP_DADD : OP_DSUB);
         if (need_value)
             PutOp(OP_DUP2_X2);
         PutOp(OP_DASTORE);
    }
    else assert(false && "unsupported PreUnary type");
}


//
// Pre Unary for which operand is field (instance variable)
// AstExpression* expression;
//
void ByteCode::EmitPreUnaryIncrementExpressionField(VariableCategory kind,
                                                    AstPreUnaryExpression* expression,
                                                    bool need_value)
{
    if (kind == ACCESSED_VAR)
        ResolveAccess(expression -> expression); // get address and value
    else
        // need to load address of object, obtained from resolution, saving
        // a copy on the stack
        EmitFieldAccessLhs(expression -> expression);

    TypeSymbol* expression_type = expression -> Type();
    if (control.IsSimpleIntegerValueType(expression_type))
    {
        PutOp(OP_ICONST_1);
        PutOp(expression -> Tag() == AstPreUnaryExpression::PLUSPLUS
              ? OP_IADD : OP_ISUB);
        EmitCast(expression_type, control.int_type);
        if (need_value)
            PutOp(OP_DUP_X1);
    }
    else if (expression_type == control.long_type)
    {
        PutOp(OP_LCONST_1);
        PutOp(expression -> Tag() == AstPreUnaryExpression::PLUSPLUS
              ? OP_LADD : OP_LSUB);
        if (need_value)
            PutOp(OP_DUP2_X1);
    }
    else if (expression_type == control.float_type)
    {
        PutOp(OP_FCONST_1);
        PutOp(expression -> Tag() == AstPreUnaryExpression::PLUSPLUS
              ? OP_FADD : OP_FSUB);
        if (need_value)
            PutOp(OP_DUP_X1);
    }
    else if (expression_type == control.double_type)
    {
        PutOp(OP_DCONST_1);
        PutOp(expression -> Tag() == AstPreUnaryExpression::PLUSPLUS
              ? OP_DADD : OP_DSUB);
        if (need_value)
            PutOp(OP_DUP2_X1);
    }
    else assert(false && "unsupported PreUnary type");

    if (kind == ACCESSED_VAR)
    {
        int stack_words = GetTypeWords(expression_type) + 1;
        PutOp(OP_INVOKESTATIC);
        CompleteCall(expression -> write_method, stack_words);
    }
    else
    {
        PutOp(OP_PUTFIELD);
        if (control.IsDoubleWordType(expression_type))
            ChangeStack(-1);

        VariableSymbol* sym = (VariableSymbol*) expression -> symbol;
        PutU2(RegisterFieldref(VariableTypeResolution(expression ->
                                                      expression, sym), sym));
    }
}


void ByteCode::EmitThisInvocation(AstThisCall* this_call)
{
    //
    // Pass enclosing instance along, then real arguments.
    //
    PutOp(OP_ALOAD_0); // load 'this'
    int stack_words = 0; // words on stack needed for arguments
    if (unit_type -> EnclosingType())
        LoadLocal(++stack_words, unit_type -> EnclosingType());
    for (unsigned k = 0; k < this_call -> arguments -> NumArguments(); k++)
        stack_words += EmitExpression(this_call -> arguments -> Argument(k));

    //
    // Now do a transfer of the shadow variables. We do not need to worry
    // about an extra null argument, as there are no accessibility issues
    // when invoking this().
    //
    if (shadow_parameter_offset)
    {
        int offset = shadow_parameter_offset;
        for (unsigned i = 0; i < unit_type -> NumConstructorParameters(); i++)
        {
            VariableSymbol* shadow = unit_type -> ConstructorParameter(i);
            LoadLocal(offset, shadow -> Type());
            int words = GetTypeWords(shadow -> Type());
            offset += words;
            stack_words += words;
        }
    }

    PutOp(OP_INVOKESPECIAL);
    ChangeStack(-stack_words);

    PutU2(RegisterMethodref(unit_type, this_call -> symbol));
}


void ByteCode::EmitSuperInvocation(AstSuperCall* super_call)
{
    //
    // Pass enclosing instance along, then real arguments, then shadow
    // variables, and finally any extra null argument for accessibility
    // issues.
    //
    PutOp(OP_ALOAD_0); // load 'this'
    int stack_words = 0; // words on stack needed for arguments
    unsigned i;
    if (super_call -> base_opt)
    {
        stack_words++;
        if (unit_type -> Anonymous())
        {
            //
            // Special case - the null check was done during the class instance
            // creation, so we skip it here.
            //
            EmitExpression(super_call -> base_opt);
        }
        else EmitCheckForNull(super_call -> base_opt);
    }
    for (i = 0; i < super_call -> arguments -> NumArguments(); i++)
        stack_words += EmitExpression(super_call -> arguments -> Argument(i));
    for (i = 0; i < super_call -> arguments -> NumLocalArguments(); i++)
        stack_words +=
            EmitExpression(super_call -> arguments -> LocalArgument(i));
    if (super_call -> arguments -> NeedsExtraNullArgument())
    {
        PutOp(OP_ACONST_NULL);
        stack_words++;
    }

    PutOp(OP_INVOKESPECIAL);
    ChangeStack(-stack_words);
    PutU2(RegisterMethodref(unit_type -> super, super_call -> symbol));
}


//
//  Methods for string concatenation
//
void ByteCode::ConcatenateString(AstBinaryExpression* expression,
                                 bool need_value)
{
    //
    // Generate code to concatenate strings, by generating a string buffer
    // and appending the arguments before calling toString, i.e.,
    //  s1+s2
    // compiles to
    //  new StringBuffer().append(s1).append(s2).toString();
    // Use recursion to share a single buffer where possible.
    // If concatenated string is not needed, we must still perform string
    // conversion on all objects, as well as perform side effects of terms.
    // In 1.5 and later, StringBuilder was added with better performance.
    //
    AstExpression* left_expr = StripNops(expression -> left_expression);
    if (left_expr -> Type() == control.String() &&
        left_expr -> BinaryExpressionCast() &&
        ! left_expr -> IsConstant())
    {
        ConcatenateString((AstBinaryExpression*) left_expr, need_value);
    }
    else
    {
        PutOp(OP_NEW);
        PutU2(RegisterClass(control.option.target >= JopaOption::SDK1_5
                            ? control.StringBuilder()
                            : control.StringBuffer()));
        PutOp(OP_DUP);
        if (left_expr -> IsConstant())
        {
            //
            // Optimizations: if the left term is "", just append the right
            // term to an empty StringBuffer. If the left term is not "",
            // use new StringBuffer(String) to create a StringBuffer
            // that includes the left term. No need to worry about
            // new StringBuffer(null) raising a NullPointerException
            // since string constants are never null.
            //
            Utf8LiteralValue* value =
                DYNAMIC_CAST<Utf8LiteralValue*> (left_expr -> value);
            if (value -> length == 0 || ! need_value)
            {
                PutOp(OP_INVOKESPECIAL);
                PutU2(RegisterLibraryMethodref
                      (control.option.target >= JopaOption::SDK1_5
                       ? control.StringBuilder_InitMethod()
                       : control.StringBuffer_InitMethod()));
            }
            else
            {
                LoadConstantAtIndex(RegisterString(value));
                PutOp(OP_INVOKESPECIAL);
                PutU2(RegisterLibraryMethodref
                      (control.option.target >= JopaOption::SDK1_5
                       ? control.StringBuilder_InitWithStringMethod()
                       : control.StringBuffer_InitWithStringMethod()));
                ChangeStack(-1); // account for the argument
            }
        }
        else
        {
            PutOp(OP_INVOKESPECIAL);
            PutU2(RegisterLibraryMethodref
                  (control.option.target >= JopaOption::SDK1_5
                   ? control.StringBuilder_InitMethod()
                   : control.StringBuffer_InitMethod()));
            //
            // Don't pass stripped left_expr, or ((int)char)+"" would be
            // treated as a char append rather than int append.
            //
            AppendString(expression -> left_expression, need_value);
        }
    }

    AppendString(expression -> right_expression, need_value);
}


void ByteCode::AppendString(AstExpression* expression, bool need_value)
{
    //
    // Grab the type before reducing no-ops, in the case of ""+(int)char.
    //
    TypeSymbol* type = expression -> Type();
    expression = StripNops(expression);

    if (expression -> IsConstant())
    {
        Utf8LiteralValue* value =
            DYNAMIC_CAST<Utf8LiteralValue*> (expression -> value);
        assert(value != NULL);
        assert(! control.IsPrimitive(type)); // Bug 2919.
        // Optimization: do nothing when appending "", or for unused result.
        if (value -> length == 0 || ! need_value)
            return;
        if (value -> length == 1)
        {
            // Optimization: append(char) more efficient than append(String)
            LoadImmediateInteger(value -> value[0]);
            type = control.char_type;
        }
        else if (value -> length == 2 &&
                 (value -> value[0] & 0x00E0) == 0x00C0)
        {
            // 2-byte string in UTF-8, but still single character.
            LoadImmediateInteger(((value -> value[0] & 0x001F) << 6) |
                                 (value -> value[1] & 0x003F));
            type = control.char_type;
        }
        else if (value -> length == 3 &&
                 (value -> value[0] & 0x00E0) == 0x00E0)
        {
            // 3-byte string in UTF-8, but still single character.
            LoadImmediateInteger(((value -> value[0] & 0x000F) << 12) |
                                 ((value -> value[1] & 0x003F) << 6) |
                                 (value -> value[2] & 0x003F));
            type = control.char_type;
        }
        else
            LoadConstantAtIndex(RegisterString(value));
    }
    else
    {
        AstBinaryExpression* binary_expression =
            expression -> BinaryExpressionCast();
        if (binary_expression && type == control.String())
        {
            assert(binary_expression -> Tag() == AstBinaryExpression::PLUS);
            AppendString(binary_expression -> left_expression, need_value);
            AppendString(binary_expression -> right_expression, need_value);
            return;
        }
        if (! need_value && control.IsPrimitive(type))
        {
            // Optimization: appending non-Object is no-op if result is unused.
            EmitExpression(expression, false);
            return;
        }
        EmitExpression(expression);
    }

    EmitStringAppendMethod(type);
}


void ByteCode::EmitStringAppendMethod(TypeSymbol* type)
{
    //
    // Find appropriate append routine to add to string buffer. Do not use
    // append(char[]), because that inserts the contents instead of the
    // correct char[].toString(). Treating null as a String is slightly more
    // efficient than as an Object.
    //
    MethodSymbol* append_method;
    if (control.option.target >= JopaOption::SDK1_5)
    {
        append_method =
            (type == control.char_type
             ? control.StringBuilder_append_charMethod()
             : type == control.boolean_type
             ? control.StringBuilder_append_booleanMethod()
             : (type == control.int_type || type == control.short_type ||
                type == control.byte_type)
             ? control.StringBuilder_append_intMethod()
             : type == control.long_type
             ? control.StringBuilder_append_longMethod()
             : type == control.float_type
             ? control.StringBuilder_append_floatMethod()
             : type == control.double_type
             ? control.StringBuilder_append_doubleMethod()
             : (type == control.String() || type == control.null_type)
             ? control.StringBuilder_append_stringMethod()
             : IsReferenceType(type)
             ? control.StringBuilder_append_objectMethod()
             : (MethodSymbol*) NULL); // for assertion
    }
    else
    {
        append_method =
            (type == control.char_type
             ? control.StringBuffer_append_charMethod()
             : type == control.boolean_type
             ? control.StringBuffer_append_booleanMethod()
             : (type == control.int_type || type == control.short_type ||
                type == control.byte_type)
             ? control.StringBuffer_append_intMethod()
             : type == control.long_type
             ? control.StringBuffer_append_longMethod()
             : type == control.float_type
             ? control.StringBuffer_append_floatMethod()
             : type == control.double_type
             ? control.StringBuffer_append_doubleMethod()
             : (type == control.String() || type == control.null_type)
             ? control.StringBuffer_append_stringMethod()
             : IsReferenceType(type)
             ? control.StringBuffer_append_objectMethod()
             : (MethodSymbol*) NULL); // for assertion
    }
    assert(append_method &&
           "unable to find method for string buffer concatenation");
    PutOp(OP_INVOKEVIRTUAL);
    if (control.IsDoubleWordType(type))
        ChangeStack(-1);
    PutU2(RegisterLibraryMethodref(append_method));
}


#ifdef JOPA_DEBUG
static void op_trap()
{
    int i = 0; // used for debugger trap
    i++;       // avoid compiler warnings about unused variable
}
#endif // JOPA_DEBUG


ByteCode::ByteCode(TypeSymbol* type)
    : ClassFile()
    , control(type -> semantic_environment -> sem -> control)
    , semantic(*type -> semantic_environment -> sem)
    , unit_type(type)
    , string_overflow(false)
    , library_method_not_found(false)
    , last_op_goto(false)
    , shadow_parameter_offset(0)
    , code_attribute(NULL)
    , line_number_table_attribute(NULL)
    , local_variable_table_attribute(NULL)
    , inner_classes_attribute(NULL)
    , double_constant_pool_index(NULL)
    , integer_constant_pool_index(NULL)
    , long_constant_pool_index(NULL)
    , float_constant_pool_index(NULL)
    , string_constant_pool_index(NULL)
    , utf8_constant_pool_index(segment_pool,
                               control.Utf8_pool.symbol_pool.Length())
    , class_constant_pool_index(segment_pool,
                                control.Utf8_pool.symbol_pool.Length())
    , name_and_type_constant_pool_index(NULL)
    , fieldref_constant_pool_index(NULL)
    , methodref_constant_pool_index(NULL)
{
#ifdef JOPA_DEBUG
    if (! control.option.nowrite)
        control.class_files_written++;
#endif // JOPA_DEBUG

    //
    // For compatibility reasons, protected classes are marked public, and
    // private classes are marked default; and no class may be static or
    // strictfp. Also, a non-access flag, the super bit, must be set for
    // classes but not interfaces. For top-level types, this changes nothing
    // except adding the super bit. For nested types, the correct access bits
    // are emitted later as part of the InnerClasses attribute. Also, no class
    // is marked strictfp.
    //
    SetFlags(unit_type -> Flags());
    if (ACC_PROTECTED())
    {
        ResetACC_PROTECTED();
        SetACC_PUBLIC();
    }
    else if (ACC_PRIVATE())
        ResetACC_PRIVATE();
    ResetACC_STATIC();
    ResetACC_STRICTFP();
    if (! unit_type -> ACC_INTERFACE())
        SetACC_SUPER();

    switch (control.option.target)
    {
    case JopaOption::SDK1_1:
        major_version = 45;
        minor_version = 3;
        break;
    case JopaOption::SDK1_2:
        major_version = 46;
        minor_version = 0;
        break;
    case JopaOption::SDK1_3:
        major_version = 47;
        minor_version = 0;
        break;
    case JopaOption::SDK1_4:
    case JopaOption::SDK1_4_2:
        major_version = 48;
        minor_version = 0;
        break;
    case JopaOption::SDK1_5:
        major_version = 49;
        minor_version = 0;
        break;
    case JopaOption::SDK1_6:
        major_version = 50;
        minor_version = 0;
        break;
    case JopaOption::SDK1_7:
        major_version = 51;
        minor_version = 0;
        break;
    case JopaOption::SDK1_8:
        major_version = 52;
        minor_version = 0;
        break;
    default:
        assert(false && "unknown version for target");
    }

#ifdef JOPA_DEBUG
    if (control.option.verbose)
	Coutput << "[generating code for class "
		<< unit_type -> fully_qualified_name -> value << " as version "
		<< major_version << '.' << minor_version << ']' << endl;
#endif // JOPA_DEBUG

    this_class = RegisterClass(unit_type);
    super_class = (unit_type -> super ? RegisterClass(unit_type -> super) : 0);
    for (unsigned k = 0; k < unit_type -> NumInterfaces(); k++)
        interfaces.Next() = RegisterClass(unit_type -> Interface(k));
}


//
//  Methods for manipulating labels
//
void ByteCode::DefineLabel(Label& lab)
{
    assert(! lab.defined && "duplicate label definition");

    //
    // Optimize if previous instruction was unconditional jump to this label.
    // However, we cannot perform the optimization if another label was also
    // defined at this location. Likewise, if local symbol tables are being
    // emitted, this optimization would screw up the symbol table.
    //
    // TODO: It would be nice to redo the bytecode emitter, to make it a
    // two-pass algorithm with straight-forward emission the first time, and
    // peephole optimizations the second time. This would be a better way to
    // cleanly collapse useless jumps, and could catch several other cases
    // that are missed or difficult to detect currently. This would require
    // creating labels at the compiled method level, rather than on the
    // invocation stack at the compiled statement level; as well as other code
    // changes. However, it might also improve inlining (such as in
    // try-finally, or in private methods); and might allow us to finally
    // implement the -O option as more than a no-op.
    //
    int index = static_cast<int>(lab.uses.Length()) - 1;
    if (last_op_goto && index >= 0 && ! (control.option.g & JopaOption::VARS))
    {
        unsigned int luse = lab.uses[index].use_offset;
        int start = luse - lab.uses[index].op_offset;
        if (start == last_op_pc &&
            code_attribute -> CodeLength() != last_label_pc)
        {
#ifdef JOPA_DEBUG
            if (control.option.debug_trace_stack_change)
                Coutput << "removing dead jump: pc " << start << endl;
#endif
            code_attribute -> DeleteCode(lab.uses[index].op_offset +
                                         lab.uses[index].use_length);
            lab.uses.Reset(index);
            line_number_table_attribute -> SetMax(start);
            last_op_goto = false;
        }
    }
    lab.defined = true;
    lab.definition = code_attribute -> CodeLength();
    if (lab.uses.Length())
        last_label_pc = lab.definition;
}


//
// patch all uses to have proper value. This requires that
// all labels be freed at some time.
//
void ByteCode::CompleteLabel(Label& lab)
{
    if (lab.uses.Length())
    {
        assert(lab.defined && "label used but with no definition");

        //
        // Sanity check - when completing method, make sure nothing jumps out
        // of the method. This also collapses two labels that begin on
        // the same location, before one is optimized away, as in
        // "if (b) <statement> else {}".
        //
        if (lab.definition > code_attribute -> CodeLength())
            lab.definition = code_attribute -> CodeLength();

        //
        // patch byte code reference to label to reflect its definition
        // as 16-bit signed offset.
        //
        for (unsigned i = 0; i < lab.uses.Length(); i++)
        {
            unsigned int luse = lab.uses[i].use_offset;
            int start = luse - lab.uses[i].op_offset,
                offset = lab.definition - start;
            if (lab.uses[i].use_length == 2) // here if short offset
            {
                assert(offset < 32768 && offset >= -32768 &&
                       "needed longer branch offset");
                code_attribute -> ResetCode(luse, (offset >> 8) & 0xFF);
                code_attribute -> ResetCode(luse + 1, offset & 0xFF);
            }
            else if (lab.uses[i].use_length == 4) // here if 4 byte use
            {
                code_attribute -> ResetCode(luse, (offset >> 24) & 0xFF);
                code_attribute -> ResetCode(luse + 1, (offset >> 16) & 0xFF);
                code_attribute -> ResetCode(luse + 2, (offset >>  8) & 0xFF);
                code_attribute -> ResetCode(luse + 3, offset & 0xFF);
            }
            else assert(false &&  "label use length not 2 or 4");
        }
    }

    //
    // reset in case label is used again.
    //
    lab.Reset();
}


void ByteCode::UseLabel(Label& lab, int _length, int _op_offset)
{
    int lab_index = lab.uses.NextIndex();
    lab.uses[lab_index].use_length = _length;
    lab.uses[lab_index].op_offset = _op_offset;
    lab.uses[lab_index].use_offset = code_attribute -> CodeLength();

    //
    // fill next length bytes with zero; will be filled in with proper value
    // when label completed
    //
    for (int i = 0; i < lab.uses[lab_index].use_length; i++)
        code_attribute -> AddCode(0);
}


void ByteCode::LoadLocal(int varno, const TypeSymbol* type)
{
    if (control.IsSimpleIntegerValueType(type) || type == control.boolean_type)
    {
         if (varno <= 3)
             PutOp((Opcode) (OP_ILOAD_0 + varno)); // Exploit opcode encodings
         else PutOpWide(OP_ILOAD, varno);
    }
    else if (type == control.long_type)
    {
         if (varno <= 3)
             PutOp((Opcode) (OP_LLOAD_0 + varno)); // Exploit opcode encodings
         else PutOpWide(OP_LLOAD, varno);
    }
    else if (type == control.float_type)
    {
         if (varno <= 3)
             PutOp((Opcode) (OP_FLOAD_0 + varno)); // Exploit opcode encodings
         else PutOpWide(OP_FLOAD, varno);
    }
    else if (type == control.double_type)
    {
         if (varno <= 3)
             PutOp((Opcode) (OP_DLOAD_0 + varno)); // Exploit opcode encodings
         else PutOpWide(OP_DLOAD, varno);
    }
    else // assume reference
    {
         if (varno <= 3)
             PutOp((Opcode) (OP_ALOAD_0 + varno)); // Exploit opcode encodings
         else PutOpWide(OP_ALOAD, varno);
    }
}


//
// See if we can load without using LDC; otherwise generate constant pool
// entry if one has not yet been generated.
//
void ByteCode::LoadLiteral(LiteralValue* litp, const TypeSymbol* type)
{
    if (control.IsSimpleIntegerValueType(type) || type == control.boolean_type)
    {
        // load literal using literal value
        IntLiteralValue* vp = DYNAMIC_CAST<IntLiteralValue*> (litp);
        LoadImmediateInteger(vp -> value);
    }
    else if (type == control.String() || type == control.null_type)
    {
        // register index as string if this has not yet been done
        Utf8LiteralValue* vp = DYNAMIC_CAST<Utf8LiteralValue*> (litp);
        LoadConstantAtIndex(RegisterString(vp));
    }
    else if (type == control.long_type)
    {
        LongLiteralValue* vp = DYNAMIC_CAST<LongLiteralValue*> (litp);
        if (vp -> value == 0)
            PutOp(OP_LCONST_0);
        else if (vp -> value == 1)
            PutOp(OP_LCONST_1);
        else if (vp -> value >= -1 && vp -> value <= 5)
        {
            LoadImmediateInteger(vp -> value.LowWord());
            PutOp(OP_I2L);
        }
        else
        {
            PutOp(OP_LDC2_W);
            PutU2(RegisterLong(vp));
        }
    }
    else if (type == control.float_type)
    {
        FloatLiteralValue* vp = DYNAMIC_CAST<FloatLiteralValue*> (litp);
        IEEEfloat val = vp -> value;
        if (val.IsZero())
        {
            PutOp(OP_FCONST_0);
            if (val.IsNegative())
                PutOp(OP_FNEG);
        }
        else if (val == 1.0f)
            PutOp(OP_FCONST_1);
        else if (val == 2.0f)
            PutOp(OP_FCONST_2);
        else if (val == -1.0f)
        {
            PutOp(OP_FCONST_1);
            PutOp(OP_FNEG);
        }
        else if (val == 3.0f || val == 4.0f || val == 5.0f)
        {
            LoadImmediateInteger(val.IntValue());
            PutOp(OP_I2F);
        }
        else LoadConstantAtIndex(RegisterFloat(vp));
    }
    else if (type == control.double_type)
    {
        DoubleLiteralValue* vp = DYNAMIC_CAST<DoubleLiteralValue*> (litp);
        IEEEdouble val = vp -> value;
        if (val.IsZero())
        {
            PutOp(OP_DCONST_0);
            if (val.IsNegative())
                PutOp(OP_DNEG);
        }
        else if (val == 1.0)
            PutOp(OP_DCONST_1);
        else if (val == -1.0)
        {
            PutOp(OP_DCONST_1);
            PutOp(OP_DNEG);
        }
        else if (val == 2.0 || val == 3.0 || val == 4.0 || val == 5.0)
        {
            LoadImmediateInteger(val.IntValue());
            PutOp(OP_I2D);
        }
        else
        {
             PutOp(OP_LDC2_W);
             PutU2(RegisterDouble(vp));
        }
    }
    else assert(false && "unsupported constant kind");
}


void ByteCode::LoadImmediateInteger(i4 val)
{
    if (val >= -1 && val <= 5)
        PutOp((Opcode) (OP_ICONST_0 + val)); // exploit opcode encoding
    else if (val >= -128 && val < 128)
    {
        PutOp(OP_BIPUSH);
        PutU1(val);
    }
    else if (val >= -32768 && val < 32768)
    {
        //
        // For a short value, look to see if it is already in the constant
        // pool. In such a case, ldc is two bytes, while sipush is three, so
        // we emit a smaller classfile with no penalty to a good JIT. Note
        // that ldc_w does not buy us anything, however.
        //
        u2 index = FindInteger(control.int_pool.Find(val));
        if (index == 0 || index > 255)
        {
            PutOp(OP_SIPUSH);
            PutU2(val);
        }
        else LoadConstantAtIndex(index);
    }
    else if (val == 65535)
    {
        PutOp(OP_ICONST_M1);
        PutOp(OP_I2C);
    }
    // Outside the range of sipush, we must use the constant pool.
    else LoadConstantAtIndex(RegisterInteger(control.int_pool.FindOrInsert(val)));
}


//
// Call to an access method for a compound operator such as ++, --,
// or "op=".
//
void ByteCode::ResolveAccess(AstExpression* p)
{
    //
    // JLS2 added ability for parenthesized variable to remain a variable.
    //
    p = StripNops(p);

    AstFieldAccess* field = p -> FieldAccessCast();
    AstExpression* resolve_expression = field ? field -> resolution_opt
        : p -> NameCast() -> resolution_opt;
    AstMethodInvocation* read_method =
        resolve_expression -> MethodInvocationCast();

    // a read method has exactly one argument: the object in question.
    assert(read_method && read_method -> arguments -> NumArguments() == 1);

    int stack_words = EmitExpression(read_method -> arguments -> Argument(0));
    PutOp(OP_DUP);
    PutOp(OP_INVOKESTATIC);
    CompleteCall(read_method -> symbol -> MethodCast(), stack_words);
}


int ByteCode::LoadVariable(VariableCategory kind, AstExpression* expr,
                           bool need_value)
{
    VariableSymbol* sym = (VariableSymbol*) expr -> symbol;
    TypeSymbol* expression_type = expr -> Type();
    AstFieldAccess* field_access = expr -> FieldAccessCast();
    AstName* name = expr -> NameCast();
    AstExpression* base = name ? name -> base_opt : field_access -> base;
    assert(field_access || name);
    switch (kind)
    {
    case LOCAL_VAR:
        assert(name && ! base);
        if (! need_value)
            return 0;
        if (expr -> IsConstant())
            LoadLiteral(expr -> value, expression_type);
        else LoadLocal(sym -> LocalVariableIndex(), expression_type);
        return GetTypeWords(expression_type);
    case ACCESSED_VAR:
        {
            //
            // A resolution is related to either this$0.field or
            // this$0.access$(). If need_value is false, and the access is
            // static, field access is smart enough to optimize away, but
            // method access requires some help.
            //
            MethodSymbol* method = expr -> symbol -> MethodCast();
            if (! need_value && method && method -> AccessesStaticMember())
                return base ? EmitExpression(base, false) : 0;
            return EmitExpression((name ? name -> resolution_opt
                                   : field_access -> resolution_opt),
                                  need_value);
        }
    case FIELD_VAR:
        assert(sym -> IsInitialized() || ! sym -> ACC_FINAL());
        if (shadow_parameter_offset && sym -> owner == unit_type &&
            (sym -> accessed_local ||
             sym -> Identity() == control.this_name_symbol))
        {
            //
            // In a constructor, use the parameter that was passed to the
            // constructor rather than the val$ or this$0 field, because the
            // field is not yet initialized.
            //
            if (! sym -> accessed_local)
            {
                PutOp(OP_ALOAD_1);
                return 1;
            }
            int offset = shadow_parameter_offset;
            for (unsigned i = 0;
                 i < unit_type -> NumConstructorParameters(); i++)
            {
                VariableSymbol* shadow = unit_type -> ConstructorParameter(i);
                if (sym == shadow)
                {
                    LoadLocal(offset, expression_type);
                    return GetTypeWords(expression_type);
                }
                offset += GetTypeWords(shadow -> Type());
            }
            assert(false && "local variable shadowing is messed up");
        }
        if (base && base -> Type() -> IsArray())
        {
            assert(sym -> name_symbol == control.length_name_symbol);
            if (base -> ArrayCreationExpressionCast() && ! need_value)
            {
                EmitExpression(base, false);
                return 0;
            }
            EmitExpression(base);
            PutOp(OP_ARRAYLENGTH);
            if (need_value)
                return 1;
            PutOp(OP_POP);
            return 0;
        }
        if (sym -> initial_value)
        {
            //
            // Inline constants without referring to the field. However, we
            // must still check for null. 
            //
            if (base)
                EmitCheckForNull(base, false);
            if (need_value)
            {
                LoadLiteral(sym -> initial_value, expression_type);
                return GetTypeWords(expression_type);
            }
            return 0;
        }
        if (base)
            EmitExpression(base);
        else PutOp(OP_ALOAD_0);
        PutOp(OP_GETFIELD);
        break;
    case STATIC_VAR:
        //
        // If the access is qualified by an arbitrary base expression,
        // evaluate it for side effects. Likewise, volatile fields must be
        // loaded because of the memory barrier side effect.
        //
        if (base)
            EmitExpression(base, false);
        if (need_value || sym -> ACC_VOLATILE())
        {
            if (sym -> initial_value)
            {
                //
                // Inline any constant. Note that volatile variables can't
                // be final, so they are not constant.
                //
                LoadLiteral(sym -> initial_value, expression_type);
                return GetTypeWords(expression_type);
            }
            PutOp(OP_GETSTATIC);
            break;
        }
        else return 0;
    default:
        assert(false && "LoadVariable bad kind");
    }
    if (control.IsDoubleWordType(expression_type))
        ChangeStack(1);
    PutU2(RegisterFieldref(VariableTypeResolution(expr, sym), sym));
    if (need_value)
    {
        return GetTypeWords(expression_type);
    }
    PutOp(control.IsDoubleWordType(expression_type) ? OP_POP2 : OP_POP);
    return 0;
}


int ByteCode::LoadArrayElement(const TypeSymbol* type)
{
    PutOp((type == control.byte_type ||
           type == control.boolean_type) ? OP_BALOAD
          : type == control.short_type ? OP_SALOAD
          : type == control.int_type ? OP_IALOAD
          : type == control.long_type ? OP_LALOAD
          : type == control.char_type ? OP_CALOAD
          : type == control.float_type ? OP_FALOAD
          : type == control.double_type ? OP_DALOAD
          : OP_AALOAD); // assume reference

    return GetTypeWords(type);
}


void ByteCode::StoreArrayElement(const TypeSymbol* type)
{
    PutOp((type == control.byte_type ||
           type == control.boolean_type) ? OP_BASTORE
          : type == control.short_type ? OP_SASTORE
          : type == control.int_type ? OP_IASTORE
          : type == control.long_type ? OP_LASTORE
          : type == control.char_type ? OP_CASTORE
          : type == control.float_type ? OP_FASTORE
          : type == control.double_type ? OP_DASTORE
          : OP_AASTORE); // assume reference
}


//
//  Method to generate field reference
//
void ByteCode::StoreField(AstExpression* expression)
{
    VariableSymbol* sym = (VariableSymbol*) expression -> symbol;
    TypeSymbol* expression_type = expression -> Type();
    if (sym -> ACC_STATIC())
    {
        PutOp(OP_PUTSTATIC);
        ChangeStack(1 - GetTypeWords(expression_type));
    }
    else
    {
        PutOp(OP_PUTFIELD);
        ChangeStack(1 - GetTypeWords(expression_type));
    }

    PutU2(RegisterFieldref(VariableTypeResolution(expression, sym), sym));
}


void ByteCode::StoreLocal(int varno, const TypeSymbol* type)
{
    if (control.IsSimpleIntegerValueType(type) || type == control.boolean_type)
    {
         if (varno <= 3)
             PutOp((Opcode) (OP_ISTORE_0 + varno)); // Exploit opcode encodings
         else PutOpWide(OP_ISTORE, varno);
    }
    else if (type == control.long_type)
    {
         if (varno <= 3)
             PutOp((Opcode) (OP_LSTORE_0 + varno)); // Exploit opcode encodings
         else PutOpWide(OP_LSTORE, varno);
    }
    else if (type == control.float_type)
    {
         if (varno <= 3)
             PutOp((Opcode) (OP_FSTORE_0 + varno)); // Exploit opcode encodings
         else PutOpWide(OP_FSTORE, varno);
    }
    else if (type == control.double_type)
    {
         if (varno <= 3)
             PutOp((Opcode) (OP_DSTORE_0 + varno)); // Exploit opcode encodings
         else PutOpWide(OP_DSTORE, varno);
    }
    else // assume reference
    {
         if (varno <= 3)
             PutOp((Opcode) (OP_ASTORE_0 + varno)); // Exploit opcode encodings
         else PutOpWide(OP_ASTORE, varno);
    }
}


void ByteCode::StoreVariable(VariableCategory kind, AstExpression* expr)
{
    VariableSymbol* sym = (VariableSymbol*) expr -> symbol;
    switch (kind)
    {
    case LOCAL_VAR:
        StoreLocal(sym -> LocalVariableIndex(), sym -> Type());
        break;
    case FIELD_VAR:
    case STATIC_VAR:
        {
            if (sym -> ACC_STATIC())
            {
                PutOp(OP_PUTSTATIC);
                ChangeStack(1 - GetTypeWords(expr -> Type()));
            }
            else
            {
                PutOp(OP_ALOAD_0); // get address of "this"
                PutOp(OP_PUTFIELD);
                ChangeStack(1 - GetTypeWords(expr -> Type()));
            }

            PutU2(RegisterFieldref(VariableTypeResolution(expr, sym), sym));
        }
        break;
    default:
        assert(false && "StoreVariable bad kind");
    }
}


//
// Finish off code by writing remaining type-level attributes.
//
void ByteCode::FinishCode()
{
    //
    // Only output SourceFile attribute if -g:source is enabled.
    //
    if (control.option.g & JopaOption::SOURCE)
        AddAttribute(new SourceFileAttribute
                     (RegisterUtf8(control.SourceFile_literal),
                      RegisterUtf8(unit_type -> file_symbol ->
                                   FileNameLiteral())));
    if (unit_type -> IsDeprecated())
        AddAttribute(CreateDeprecatedAttribute());
    if (unit_type -> ACC_SYNTHETIC() &&
        control.option.target < JopaOption::SDK1_5)
    {
        AddAttribute(CreateSyntheticAttribute());
    }
    if (unit_type -> owner -> MethodCast())
    {
        MethodSymbol* enclosing = (MethodSymbol*) unit_type -> owner;
        AddAttribute(CreateEnclosingMethodAttribute(enclosing));
    }

    // Add Signature attribute for generic classes
    if (unit_type -> IsGeneric())
    {
        unit_type -> SetGenericSignature(control);
        if (unit_type -> GenericSignature())
        {
            AddAttribute(new SignatureAttribute(
                RegisterUtf8(control.Signature_literal),
                RegisterUtf8(unit_type -> GenericSignature())));
        }
    }

    //
    // Process RuntimeVisibleAnnotations for classes
    //
    AstClassDeclaration* class_decl = NULL;
    AstInterfaceDeclaration* interface_decl = NULL;
    AstEnumDeclaration* enum_decl = NULL;
    AstAnnotationDeclaration* annotation_decl = NULL;

    if (unit_type -> declaration && unit_type -> declaration -> owner)
    {
        class_decl = unit_type -> declaration -> owner -> ClassDeclarationCast();
        if (!class_decl)
            interface_decl = unit_type -> declaration -> owner -> InterfaceDeclarationCast();
        if (!class_decl && !interface_decl)
            enum_decl = unit_type -> declaration -> owner -> EnumDeclarationCast();
        if (!class_decl && !interface_decl && !enum_decl)
            annotation_decl = unit_type -> declaration -> owner -> AnnotationDeclarationCast();
    }

    AstModifiers* class_mods = class_decl ? class_decl -> modifiers_opt :
                               interface_decl ? interface_decl -> modifiers_opt :
                               enum_decl ? enum_decl -> modifiers_opt :
                               annotation_decl ? annotation_decl -> modifiers_opt : NULL;

    if (class_mods)
    {
        AnnotationsAttribute* annotations_attr = NULL;

        for (unsigned i = 0; i < class_mods -> NumModifiers(); i++)
        {
            AstAnnotation* annotation = class_mods -> Modifier(i) -> AnnotationCast();
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
            AddAttribute(annotations_attr);
        }
    }

    //
    // In case they weren't referenced elsewhere, make sure all nested types
    // of this class are listed in the constant pool.  A side effect of
    // registering the class is updating the InnerClasses attribute.
    //
    unsigned i = unit_type -> NumNestedTypes();
    while (i--)
        RegisterClass(unit_type -> NestedType(i));
}


void ByteCode::PutOp(Opcode opc)
{
#ifdef JOPA_DEBUG
    if (control.option.debug_trap_op &&
        code_attribute -> CodeLength() == (u2) control.option.debug_trap_op)
    {
        op_trap();
    }

    if (control.option.debug_trace_stack_change)
    {
        const char* opname;
        OpDesc(opc, &opname, NULL);
        Coutput << "opcode: " << opname << endl;
    }
#endif // JOPA_DEBUG

    // save pc at start of operation
    last_op_pc = code_attribute -> CodeLength();
    code_attribute -> AddCode(opc);
    ChangeStack(stack_effect[opc]);
    last_op_goto = (opc == OP_GOTO || opc == OP_GOTO_W);
}

void ByteCode::PutOpWide(Opcode opc, u2 var)
{
    if (var <= 255)  // if can use standard form
    {
        PutOp(opc);
        PutU1(var);
    }
    else // need wide form
    {
        PutOp(OP_WIDE);
        PutOp(opc);
        PutU2(var);
    }
}

void ByteCode::PutOpIINC(u2 var, int val)
{
    if (var <= 255 && (val >= -128 && val <= 127))  // if can use standard form
    {
        PutOp(OP_IINC);
        PutU1(var);
        PutU1(val);
    }
    else // else need wide form
    {
        PutOp(OP_WIDE);
        PutOp(OP_IINC);
        PutU2(var);
        PutU2(val);
    }
}

void ByteCode::ChangeStack(int i)
{
    stack_depth += i;
    assert(stack_depth >= 0);

    if (i > 0 && stack_depth > max_stack)
        max_stack = stack_depth;

#ifdef JOPA_DEBUG
    if (control.option.debug_trace_stack_change)
        Coutput << "stack change: pc " << last_op_pc << " change " << i
                << "  stack_depth " << stack_depth << "  max_stack: "
                << max_stack << endl;
#endif // JOPA_DEBUG
}



} // Close namespace Jopa block

