// ByteCode expression emission
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
    // Java 5: For static imports, base is NULL but we should use the
    // containing type, not unit_type, so the correct class is referenced.
    //
    TypeSymbol* owner_type = sym -> ContainingType();
    TypeSymbol* candidate = field ? field -> base -> Type()
        : name -> base_opt ? name -> base_opt -> Type()
        : sym -> ACC_STATIC() ? owner_type  // Java 5: static import
        : unit_type;
    return (owner_type -> ACC_INTERFACE() &&
            control.option.target < JopaOption::SDK1_4)
        ? owner_type : candidate;
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

    // Record StackMapTable frame for class$ exception handler (at offset 12)
    if (stack_map_generator)
    {
        stack_map_generator->ClearStack();
        stack_map_generator->PushType(control.ClassNotFoundException());
        stack_map_generator->RecordFrame(12);
        stack_map_generator->ClearStack(); // Reset for subsequent non-handler code
    }

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

    // Track stack words for parameters (for ChangeStack adjustment after invoke)
    int stack_words = 0;

    // Load 'this' if not static
    if (! bridge -> ACC_STATIC())
    {
        PutOp(OP_ALOAD_0);
        stack_words++;
    }

    // Load all parameters and cast if needed for generic bridge
    u2 local_index = bridge -> ACC_STATIC() ? 0 : 1;
    for (unsigned i = 0; i < bridge -> NumFormalParameters(); i++)
    {
        VariableSymbol* bridge_param = bridge -> FormalParameter(i);
        TypeSymbol* bridge_param_type = bridge_param -> Type();
        TypeSymbol* target_param_type = target -> FormalParameter(i) -> Type();

        if (control.IsSimpleIntegerValueType(bridge_param_type) ||
            bridge_param_type == control.boolean_type)
        {
            LoadLocal(local_index, bridge_param_type);
            local_index++;
            stack_words++;
        }
        else if (bridge_param_type == control.long_type)
        {
            LoadLocal(local_index, bridge_param_type);
            local_index += 2;
            stack_words += 2;
        }
        else if (bridge_param_type == control.float_type)
        {
            LoadLocal(local_index, bridge_param_type);
            local_index++;
            stack_words++;
        }
        else if (bridge_param_type == control.double_type)
        {
            LoadLocal(local_index, bridge_param_type);
            local_index += 2;
            stack_words += 2;
        }
        else // reference type
        {
            LoadLocal(local_index, bridge_param_type);
            local_index++;
            stack_words++;

            // If target expects a different type, add a cast
            // This handles generic bridges like Object->String
            if (target_param_type != bridge_param_type &&
                ! bridge_param_type -> IsSubtype(target_param_type))
            {
                PutOp(OP_CHECKCAST);
                PutU2(RegisterClass(target_param_type));
            }
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
        PutU1(stack_words); // count includes 'this' and all args
        PutU1(0);
    }
    else
    {
        PutOp(OP_INVOKEVIRTUAL);
        PutU2(RegisterMethodref(target -> containing_type, target));
    }

    // Adjust stack: pop receiver + args, push return value
    // The PutOp already applied a default stack effect (-1 for INVOKEVIRTUAL),
    // so we need to adjust for the actual method signature
    ChangeStack(-stack_words + 1); // +1 because INVOKEVIRTUAL already did -1

    TypeSymbol* return_type = bridge -> Type();
    if (return_type != control.void_type)
    {
        // Push the return value
        bool wide = control.IsDoubleWordType(target -> Type());
        ChangeStack(wide ? 2 : 1);
    }

    // Return the result
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
    if (!expression->symbol) {
        fprintf(stderr, "INTERNAL ERROR: Symbol is NULL in EmitName. Skipping.\n");
        return 0;
    }

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

            // Java 5: Handle wrapper types by unboxing
            TypeSymbol* unboxed_op_type = op_type -> UnboxedType(control);
            if (unboxed_op_type != op_type && (control.IsNumeric(unboxed_op_type) ||
                                               unboxed_op_type == control.boolean_type))
            {
                op_type = unboxed_op_type;
            }

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
            // "Assume false, update if true" pattern:
            //   iconst_0              ; push 0 (assume false)
            //   emit_branch_if(expr, false, MERGE)  ; if expr is false, jump to MERGE
            //   pop                   ; remove the 0
            //   iconst_1              ; push 1 (true)
            //   MERGE:                ; result on stack
            //
            // At MERGE, both paths have: [...stuff, int] where int is 0 or 1.
            // This pattern has only ONE branch target (MERGE), not two.
            //
            // For Java 7+ bytecode, we mark this as no_frame because the stack
            // tracking for internal expression labels is incomplete. Tests pass
            // with strict verification for targets 1.5 and 1.6.
            Label label;
            label.no_frame = true;
            PutOp(OP_ICONST_0); // push false (assume false)
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
    // Also handles boxing followed by widening (e.g., int → Integer → Object)
    if (control.option.source >= JopaOption::SDK1_5 &&
        source_type -> Primitive())
    {
        TypeSymbol* wrapper = semantic.GetWrapperType(source_type);
        if (wrapper && wrapper -> IsSubtype(dest_type))
        {
            // Box to wrapper type (e.g., int → Integer)
            EmitBoxingConversion(source_type, wrapper);
            // Widening from wrapper to dest is implicit (Integer → Object)
            return;
        }
    }

    // Java 5: Handle unboxing conversion (wrapper → primitive)
    // General case: Unbox any wrapper to its primitive, then convert primitive to dest
    if (control.option.source >= JopaOption::SDK1_5 &&
        !source_type->Primitive() && dest_type->Primitive())
    {
        TypeSymbol* source_primitive = semantic.GetPrimitiveType(source_type);
        if (source_primitive)
        {
            EmitUnboxingConversion(source_type, source_primitive);
            // If types match, we are done. If not (e.g. byte -> float), emit conversion.
            if (source_primitive != dest_type)
                EmitCast(dest_type, source_primitive);
            return;
        }
    }

    // Java 5: General Unboxing (Reference -> Primitive)
    // Includes casting Object/Number/Interface to wrapper then unboxing
    // e.g. (int)Object -> CHECKCAST Integer -> intValue()
    if (control.option.source >= JopaOption::SDK1_5 &&
        dest_type -> Primitive() && !source_type -> Primitive())
    {
        TypeSymbol* wrapper = semantic.GetWrapperType(dest_type);
        if (wrapper)
        {
            // Cast source to wrapper (recursive call will emit CHECKCAST wrapper)
            EmitCast(wrapper, source_type);
            // Unbox wrapper to primitive
            EmitUnboxingConversion(wrapper, dest_type);
            return;
        }
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
    // If constructor is NULL (unresolved due to semantic errors), skip code generation
    if (! constructor)
        return need_value ? 1 : 0;
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

            // Generate: newarray/anewarray <component_type> with count 0
            PutOp(OP_ICONST_0);
            if (control.IsPrimitive(component_type))
            {
                PutOp(OP_NEWARRAY);
                PutU1(component_type == control.boolean_type ? 4
                      : component_type == control.char_type ? 5
                      : component_type == control.float_type ? 6
                      : component_type == control.double_type ? 7
                      : component_type == control.byte_type ? 8
                      : component_type == control.short_type ? 9
                      : component_type == control.int_type ? 10
                      : 11); // control.long_type
            }
            else
            {
                PutOp(OP_ANEWARRAY);
                PutU2(RegisterClass(component_type));
            }
            ChangeStack(0); // iconst_0 (+1), newarray/anewarray uses it and produces array (+0 net)
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
    int result = CompleteCall(msym, stack_words, need_value, type);

    // Java 5+: For generic methods, emit checkcast if the method's erased return type
    // (e.g., Object) differs from the expression's declared type (e.g., Integer).
    // This is required for bytecode verification since the verifier only sees the
    // erased signature but the calling code expects the parameterized type.
    if (need_value && msym -> Type() != control.void_type)
    {
        TypeSymbol* erased_return_type = msym -> Type();
        TypeSymbol* declared_type = expression -> Type();

        // Check if cast is needed: erased type differs from declared type,
        // and declared type is more specific (not Object itself)
        if (erased_return_type != declared_type &&
            ! control.IsPrimitive(declared_type) &&
            declared_type != control.Object() &&
            erased_return_type -> IsSubtype(control.Object()) &&
            ! erased_return_type -> IsSubtype(declared_type))
        {
            PutOp(OP_CHECKCAST);
            PutU2(RegisterClass(declared_type));
        }
    }

    return result;
}


int ByteCode::CompleteCall(MethodSymbol* msym, int stack_words,
                           bool need_value, TypeSymbol* base_type)
{
    // If method is NULL (unresolved due to semantic errors), skip code generation
    if (! msym)
        return need_value ? 1 : 0;
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

} // Close namespace Jopa block
