// ByteCode load/store operations and low-level bytecode emission
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
    // If symbol is NULL (unresolved due to semantic errors), skip code generation
    if (! this_call -> symbol)
        return;
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
    // If symbol is NULL (unresolved due to semantic errors), skip code generation
    if (! super_call -> symbol)
        return;
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
    // Breakpoint location for debugger
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

    //
    // Save the current locals state at definition time (for backward branches).
    // This captures the locals before any inner-scope variables are declared.
    // When a backward branch is made to this label later, we use this saved
    // state instead of the current state which may include out-of-scope locals.
    //
    if (stack_map_generator && !lab.no_frame && !lab.locals_saved)
    {
        lab.saved_locals_types = stack_map_generator->SaveLocals();
        lab.locals_saved = true;
    }

    //
    // Record StackMapTable frame at branch targets (Java 7+)
    // A label with forward references (uses) indicates a branch target that
    // needs a stack map frame for verification.
    // Skip if no_frame is set (internal expression labels like boolean-to-value pattern).
    //
    // For forward branches, we have saved_locals from the earliest branch point.
    // We also have current_locals at the definition point.
    // Use the SHORTER of the two - this handles:
    // 1. If-else: saved_locals has fewer (before if-branch vars) - use saved
    // 2. Break from catch: saved_locals has more (includes exception) - use current
    //
    if (stack_map_generator && lab.uses.Length() > 0 && !lab.no_frame)
    {
        Tuple<StackMapTableAttribute::VerificationTypeInfo>* current_locals = stack_map_generator->SaveLocals();
        Tuple<StackMapTableAttribute::VerificationTypeInfo>* locals_to_use = NULL;

#ifdef JOPA_DEBUG
        if (control.option.debug_trace_stack_change)
        {
            Coutput << "DefineLabel at pc " << lab.definition
                    << " uses=" << lab.uses.Length()
                    << " saved_len=" << (lab.saved_locals_types ? lab.saved_locals_types->Length() : 0)
                    << " current_len=" << current_locals->Length() << endl;
        }
#endif

        // Compute the intersection of saved_locals and current_locals.
        // At a merge point, each local slot must be valid on ALL paths.
        // If any path has Top (undefined) at a slot, the result must be Top.
        // We also want the minimum "effective length" - the last slot that is
        // defined (non-Top) on ALL paths.
        if (lab.locals_saved && lab.saved_locals_types)
        {
            // Create intersection result
            unsigned min_len = (lab.saved_locals_types->Length() < current_locals->Length())
                ? lab.saved_locals_types->Length() : current_locals->Length();

            locals_to_use = new Tuple<StackMapTableAttribute::VerificationTypeInfo>(min_len, 8);

            // Find the last slot where BOTH have non-Top types
            unsigned effective_len = 0;
            for (unsigned i = 0; i < min_len; i++)
            {
                StackMapTableAttribute::VerificationTypeInfo saved_type = (*lab.saved_locals_types)[i];
                StackMapTableAttribute::VerificationTypeInfo current_type = (*current_locals)[i];

                // If either is Top, result should be Top (or we can stop here for effective length)
                if (saved_type.Tag() == StackMapTableAttribute::VerificationTypeInfo::TYPE_Top ||
                    current_type.Tag() == StackMapTableAttribute::VerificationTypeInfo::TYPE_Top)
                {
                    // Include Top entries only if there are more non-Top entries after
                    // For simplicity, we compute effective_len separately below
                    locals_to_use->Next() = StackMapTableAttribute::VerificationTypeInfo(
                        StackMapTableAttribute::VerificationTypeInfo::TYPE_Top);
                }
                else
                {
                    // Both have defined types - use one of them (they should match for valid code)
                    locals_to_use->Next() = current_type;
                    effective_len = i + 1;
                }
            }

            // Trim to effective length (last non-Top entry + 1)
            // This removes trailing Top entries from the frame
            while (locals_to_use->Length() > effective_len)
                locals_to_use->Reset(locals_to_use->Length() - 1);
        }
        else
        {
            locals_to_use = current_locals;
            current_locals = NULL; // Don't delete, we're using it
        }

        if (lab.stack_saved && lab.saved_stack_types)
        {
#ifdef JOPA_DEBUG
            if (control.option.debug_trace_stack_change)
                Coutput << "  -> RecordFrameWithSavedLocalsAndStack pc=" << lab.definition
                        << " locals=" << locals_to_use->Length() << endl;
#endif
            stack_map_generator->RecordFrameWithSavedLocalsAndStack(
                lab.definition, locals_to_use, lab.saved_stack_types);
        }
        else
        {
#ifdef JOPA_DEBUG
            if (control.option.debug_trace_stack_change)
                Coutput << "  -> RecordFrameWithLocals pc=" << lab.definition
                        << " locals=" << locals_to_use->Length() << endl;
#endif
            stack_map_generator->RecordFrameWithLocals(lab.definition, locals_to_use);
        }
        if (locals_to_use != current_locals)
            delete locals_to_use;
        if (current_locals)
            delete current_locals;
    }
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
    // For forward branches (label not yet defined), save the current stack state.
    // This is needed for StackMapTable generation - the frame at the branch target
    // should reflect the stack types at the branch point (before the branch instruction
    // pops its operands).
    //
    // Also save locals on the FIRST forward branch use. This captures the locals
    // at the earliest branch source, before any block-scoped variables are declared
    // in subsequent code. For if-else branches, this ensures the else-branch frame
    // has correct locals (without if-branch variables).
    //
    // Exception handlers are a special case - they set no_frame to skip frame recording,
    // or handle frames specially through EmitBranch's exception_label parameter.
    //
    if (stack_map_generator && !lab.defined && !lab.stack_saved)
    {
        lab.saved_stack_depth = stack_depth;
        lab.saved_stack_types = stack_map_generator->SaveStack();
        lab.stack_saved = true;
    }
    // For forward branches, maintain the INTERSECTION of locals from all paths.
    // On first use, save current locals. On subsequent uses, intersect with current.
    // This ensures the frame at the target only includes locals valid on ALL paths.
    if (stack_map_generator && !lab.defined)
    {
        Tuple<StackMapTableAttribute::VerificationTypeInfo>* current_locals = stack_map_generator->SaveLocals();

        if (!lab.locals_saved)
        {
            // First forward branch - just save current locals
            lab.saved_locals_types = current_locals;
            lab.locals_saved = true;
#ifdef JOPA_DEBUG
            if (control.option.debug_trace_stack_change)
                Coutput << "UseLabel: first use, saved_len=" << current_locals->Length() << endl;
#endif
        }
        else
        {
            // Subsequent forward branch - compute intersection with saved locals
            // A local is valid only if it's defined on ALL paths to this label
            unsigned min_len = (lab.saved_locals_types->Length() < current_locals->Length())
                ? lab.saved_locals_types->Length() : current_locals->Length();

            // Find effective length (last slot where BOTH paths have non-Top)
            unsigned effective_len = 0;
            for (unsigned i = 0; i < min_len; i++)
            {
                StackMapTableAttribute::VerificationTypeInfo saved_type = (*lab.saved_locals_types)[i];
                StackMapTableAttribute::VerificationTypeInfo current_type = (*current_locals)[i];

                if (saved_type.Tag() != StackMapTableAttribute::VerificationTypeInfo::TYPE_Top &&
                    current_type.Tag() != StackMapTableAttribute::VerificationTypeInfo::TYPE_Top)
                {
                    effective_len = i + 1;
                }
            }

#ifdef JOPA_DEBUG
            if (control.option.debug_trace_stack_change)
                Coutput << "UseLabel: intersection saved_len=" << lab.saved_locals_types->Length()
                        << " current_len=" << current_locals->Length()
                        << " min_len=" << min_len
                        << " effective_len=" << effective_len << endl;
#endif

            // Truncate saved_locals to the intersection
            while (lab.saved_locals_types->Length() > effective_len)
                lab.saved_locals_types->Reset(lab.saved_locals_types->Length() - 1);

            delete current_locals;
        }
    }

    //
    // For backward branches (label already defined), we need to record a frame
    // at the target if one hasn't been recorded already. This is critical for
    // loop back-edges where the target was defined before any branches to it.
    //
    // IMPORTANT: Use the saved locals state from when the label was defined,
    // NOT the current state. The current state may include inner-scope variables
    // that don't exist on all paths to the label (e.g., loop variables declared
    // inside a loop body when branching back to the loop header).
    //
    if (stack_map_generator && lab.defined && !lab.no_frame)
    {
        // Record a frame at the backward branch target with empty stack
        if (!stack_map_generator->HasFrameAt(lab.definition))
        {
            if (lab.locals_saved && lab.saved_locals_types)
            {
                // Use the locals state from when the label was defined
                stack_map_generator->RecordFrameWithLocals(lab.definition, lab.saved_locals_types);
            }
            else
            {
                // Fallback: use current state (should rarely happen)
                stack_map_generator->ClearStack();
                stack_map_generator->RecordFrame(lab.definition);
            }
        }
    }

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
            LoadImmediateInteger(static_cast<i4>(vp -> value.LowWord()));
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
        PutU1(static_cast<u1>(val));  // BIPUSH interprets byte as signed
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
            PutU2(static_cast<u2>(val));  // SIPUSH interprets short as signed
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
    // Track local variable type for StackMapTable
    if (stack_map_generator)
    {
        stack_map_generator->SetLocal(varno, const_cast<TypeSymbol*>(type));
        // For long/double, set second slot to TOP
        if (type == control.long_type || type == control.double_type)
        {
            stack_map_generator->SetLocal(varno + 1,
                StackMapTableAttribute::VerificationTypeInfo(
                    StackMapTableAttribute::VerificationTypeInfo::TYPE_Top));
        }
    }

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

    // Add Signature attribute for generic classes and classes with parameterized superclasses (Java 5+)
    // (the latter is needed for TypeToken pattern and similar reflection-based code)
    if (control.option.target >= JopaOption::SDK1_5 &&
        (unit_type -> IsGeneric() || unit_type -> HasParameterizedSuper()))
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
    // Process RuntimeVisibleAnnotations for classes (Java 5+)
    //
    if (control.option.target >= JopaOption::SDK1_5)
    {
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
    } // end SDK1_5+ annotations guard

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
        PutU1(static_cast<u1>(val));  // IINC interprets byte as signed
    }
    else // else need wide form
    {
        PutOp(OP_WIDE);
        PutOp(OP_IINC);
        PutU2(var);
        PutU2(static_cast<u2>(val));  // Wide IINC stores signed 16-bit, cast to u2 for bit pattern
    }
}

void ByteCode::ChangeStack(int i)
{
    stack_depth += i;
    // In error recovery scenarios, stack depth may go negative.
    // Instead of asserting, clamp to 0 to avoid further issues.
    if (stack_depth < 0)
    {
        stack_depth = 0;
    }

    if (i > 0 && stack_depth > max_stack)
        max_stack = stack_depth;

#ifdef JOPA_DEBUG
    if (control.option.debug_trace_stack_change)
        Coutput << "stack change: pc " << last_op_pc << " change " << i
                << "  stack_depth " << stack_depth << "  max_stack: "
                << max_stack << endl;
#endif // JOPA_DEBUG
}


//
// StackMapGenerator implementation
//

void StackMapGenerator::InitializeMethod(MethodSymbol* method, TypeSymbol* type)
{
    Reset();
    containing_type = type;
    is_constructor = method->Identity() == control.init_name_symbol;
    is_instance_method = !method->ACC_STATIC();
    super_called = !is_constructor; // Non-constructors start with 'this' initialized

    u2 local_index = 0;

    // Instance methods have 'this' in slot 0
    if (is_instance_method)
    {
        if (is_constructor && !super_called)
        {
            // In constructor before super(), 'this' is uninitialized
            SetLocal(local_index, VerificationType(VerificationType::TYPE_UninitializedThis));
        }
        else
        {
            SetLocal(local_index, type);
        }
        local_index++;
    }

    // Add parameters
    for (unsigned i = 0; i < method->NumFormalParameters(); i++)
    {
        VariableSymbol* param = method->FormalParameter(i);
        TypeSymbol* param_type = param->Type();
        SetLocal(local_index, param_type);
        local_index++;

        // Long and double take two slots
        if (param_type == control.long_type || param_type == control.double_type)
        {
            SetLocal(local_index, VerificationType(VerificationType::TYPE_Top));
            local_index++;
        }
    }

    max_locals_tracked = local_index;
}

StackMapGenerator::VerificationType StackMapGenerator::TypeToVerificationType(TypeSymbol* type)
{
    if (!type)
        return VerificationType(VerificationType::TYPE_Top);

    if (type == control.int_type ||
        type == control.short_type ||
        type == control.byte_type ||
        type == control.char_type ||
        type == control.boolean_type)
    {
        return VerificationType(VerificationType::TYPE_Integer);
    }
    else if (type == control.float_type)
    {
        return VerificationType(VerificationType::TYPE_Float);
    }
    else if (type == control.double_type)
    {
        return VerificationType(VerificationType::TYPE_Double);
    }
    else if (type == control.long_type)
    {
        return VerificationType(VerificationType::TYPE_Long);
    }
    else if (type == control.null_type)
    {
        return VerificationType(VerificationType::TYPE_Null);
    }
    else if (type->Primitive())
    {
        // void or other - should not happen for verification types
        return VerificationType(VerificationType::TYPE_Top);
    }
    else
    {
        // Reference type - need constant pool index
        // We use RegisterClass from ByteCode, but we need to store just the index
        // For now, use 0 as placeholder - will be resolved when generating attribute
        u2 class_index = bytecode.RegisterClass(type);
        return VerificationType(VerificationType::TYPE_Object, class_index);
    }
}

void StackMapGenerator::PushType(const VerificationType& type)
{
    current_stack.Next() = type;
}

void StackMapGenerator::PushType(TypeSymbol* type)
{
    PushType(TypeToVerificationType(type));
}

void StackMapGenerator::PopType()
{
    if (current_stack.Length() > 0)
        current_stack.Reset(current_stack.Length() - 1);
}

void StackMapGenerator::PopTypes(int count)
{
    for (int i = 0; i < count && current_stack.Length() > 0; i++)
        current_stack.Reset(current_stack.Length() - 1);
}

void StackMapGenerator::ClearStack()
{
    current_stack.Reset();
}

void StackMapGenerator::SetLocal(u2 index, const VerificationType& type)
{
    // Extend locals array if needed
    while (current_locals.Length() <= index)
    {
        current_locals.Next() = VerificationType(VerificationType::TYPE_Top);
    }
    current_locals[index] = type;

    if (index >= max_locals_tracked)
        max_locals_tracked = index + 1;
}

void StackMapGenerator::SetLocal(u2 index, TypeSymbol* type)
{
    SetLocal(index, TypeToVerificationType(type));
}

StackMapGenerator::VerificationType StackMapGenerator::GetLocal(u2 index) const
{
    if (index < current_locals.Length())
        return current_locals[index];
    return VerificationType(VerificationType::TYPE_Top);
}

void StackMapGenerator::RecordFrame(u2 pc)
{
    // Don't record duplicate frames at the same PC
    if (HasFrameAt(pc))
        return;

    FrameEntry* entry = new FrameEntry(pc);

    // Copy current locals
    for (unsigned i = 0; i < current_locals.Length(); i++)
        entry->locals.Next() = current_locals[i];

    // Copy current stack
    for (unsigned i = 0; i < current_stack.Length(); i++)
        entry->stack.Next() = current_stack[i];

    // Insert in sorted order by PC
    unsigned insert_pos = recorded_frames.Length();
    for (unsigned i = 0; i < recorded_frames.Length(); i++)
    {
        if (recorded_frames[i]->pc > pc)
        {
            insert_pos = i;
            break;
        }
    }

    // Make room and insert
    if (insert_pos == recorded_frames.Length())
    {
        recorded_frames.Next() = entry;
    }
    else
    {
        // Shift elements to make room
        recorded_frames.Next() = NULL; // Extend array
        for (unsigned i = recorded_frames.Length() - 1; i > insert_pos; i--)
            recorded_frames[i] = recorded_frames[i - 1];
        recorded_frames[insert_pos] = entry;
    }
}

void StackMapGenerator::RecordFrameWithLocals(u2 pc, Tuple<VerificationType>* saved_locals)
{
    // Check if there's already a frame at this PC
    for (unsigned i = 0; i < recorded_frames.Length(); i++)
    {
        if (recorded_frames[i]->pc == pc)
        {
            // Frame exists - update it if new locals count is smaller
            // This handles the case where multiple labels target the same PC,
            // and we need to use the intersection (minimum) of all paths
            if (saved_locals->Length() < recorded_frames[i]->locals.Length())
            {
#ifdef JOPA_DEBUG
                if (control.option.debug_trace_stack_change)
                    Coutput << "RecordFrameWithLocals: UPDATING at pc " << pc
                            << " from " << recorded_frames[i]->locals.Length()
                            << " to " << saved_locals->Length() << " locals" << endl;
#endif
                // Clear and rebuild locals list with fewer entries
                while (recorded_frames[i]->locals.Length() > 0)
                    recorded_frames[i]->locals.Reset(recorded_frames[i]->locals.Length() - 1);
                for (unsigned j = 0; j < saved_locals->Length(); j++)
                    recorded_frames[i]->locals.Next() = (*saved_locals)[j];
            }
#ifdef JOPA_DEBUG
            else if (control.option.debug_trace_stack_change)
            {
                Coutput << "RecordFrameWithLocals: SKIPPING at pc " << pc
                        << " (existing " << recorded_frames[i]->locals.Length()
                        << " <= new " << saved_locals->Length() << ")" << endl;
            }
#endif
            return;
        }
    }

#ifdef JOPA_DEBUG
    if (control.option.debug_trace_stack_change)
        Coutput << "RecordFrameWithLocals: recording at pc " << pc
                << " with " << saved_locals->Length() << " locals" << endl;
#endif

    FrameEntry* entry = new FrameEntry(pc);

    // Copy saved locals
    for (unsigned i = 0; i < saved_locals->Length(); i++)
        entry->locals.Next() = (*saved_locals)[i];

    // Stack is empty for this frame type

    // Insert in sorted order by PC
    unsigned insert_pos = recorded_frames.Length();
    for (unsigned i = 0; i < recorded_frames.Length(); i++)
    {
        if (recorded_frames[i]->pc > pc)
        {
            insert_pos = i;
            break;
        }
    }

    // Make room and insert
    if (insert_pos == recorded_frames.Length())
    {
        recorded_frames.Next() = entry;
    }
    else
    {
        // Shift elements to make room
        recorded_frames.Next() = NULL; // Extend array
        for (unsigned i = recorded_frames.Length() - 1; i > insert_pos; i--)
            recorded_frames[i] = recorded_frames[i - 1];
        recorded_frames[insert_pos] = entry;
    }
}

void StackMapGenerator::RecordFrameWithLocalsAndStack(u2 pc, Tuple<VerificationType>* saved_locals,
                                                      TypeSymbol* stack_type)
{
    // Don't record duplicate frames at the same PC
    if (HasFrameAt(pc))
        return;

    FrameEntry* entry = new FrameEntry(pc);

    // Copy saved locals
    for (unsigned i = 0; i < saved_locals->Length(); i++)
        entry->locals.Next() = (*saved_locals)[i];

    // Add the stack type
    entry->stack.Next() = TypeToVerificationType(stack_type);

    // Insert in sorted order by PC
    unsigned insert_pos = recorded_frames.Length();
    for (unsigned i = 0; i < recorded_frames.Length(); i++)
    {
        if (recorded_frames[i]->pc > pc)
        {
            insert_pos = i;
            break;
        }
    }

    // Make room and insert
    if (insert_pos == recorded_frames.Length())
    {
        recorded_frames.Next() = entry;
    }
    else
    {
        // Shift elements to make room
        recorded_frames.Next() = NULL; // Extend array
        for (unsigned i = recorded_frames.Length() - 1; i > insert_pos; i--)
            recorded_frames[i] = recorded_frames[i - 1];
        recorded_frames[insert_pos] = entry;
    }
}

void StackMapGenerator::TruncateLocals(unsigned from_index)
{
    // Set all locals from from_index onwards to TOP
    // This effectively "truncates" the locals array for verification purposes
    for (unsigned i = from_index; i < current_locals.Length(); i++)
    {
        current_locals[i] = VerificationType(VerificationType::TYPE_Top);
    }
}

Tuple<StackMapGenerator::VerificationType>* StackMapGenerator::SaveStack()
{
    Tuple<VerificationType>* saved = new Tuple<VerificationType>(current_stack.Length() + 2);
    for (unsigned i = 0; i < current_stack.Length(); i++)
        saved->Next() = current_stack[i];
    return saved;
}

void StackMapGenerator::RecordFrameWithSavedStack(u2 pc, Tuple<VerificationType>* saved_stack)
{
    // Don't record duplicate frames at the same PC
    if (HasFrameAt(pc))
        return;

    FrameEntry* entry = new FrameEntry(pc);

    // Copy current locals
    for (unsigned i = 0; i < current_locals.Length(); i++)
        entry->locals.Next() = current_locals[i];

    // Copy saved stack
    if (saved_stack)
    {
        for (unsigned i = 0; i < saved_stack->Length(); i++)
            entry->stack.Next() = (*saved_stack)[i];
    }

    // Insert in sorted order by PC
    unsigned insert_pos = recorded_frames.Length();
    for (unsigned i = 0; i < recorded_frames.Length(); i++)
    {
        if (recorded_frames[i]->pc > pc)
        {
            insert_pos = i;
            break;
        }
    }

    // Make room and insert
    if (insert_pos == recorded_frames.Length())
    {
        recorded_frames.Next() = entry;
    }
    else
    {
        // Shift elements to make room
        recorded_frames.Next() = NULL; // Extend array
        for (unsigned i = recorded_frames.Length() - 1; i > insert_pos; i--)
            recorded_frames[i] = recorded_frames[i - 1];
        recorded_frames[insert_pos] = entry;
    }
}

void StackMapGenerator::RecordFrameWithSavedLocalsAndStack(u2 pc, Tuple<VerificationType>* saved_locals,
                                                           Tuple<VerificationType>* saved_stack)
{
    unsigned new_locals_len = saved_locals ? saved_locals->Length() : 0;

    // Check if there's already a frame at this PC
    for (unsigned i = 0; i < recorded_frames.Length(); i++)
    {
        if (recorded_frames[i]->pc == pc)
        {
            // Frame exists - update it if new locals count is smaller
            if (new_locals_len < recorded_frames[i]->locals.Length())
            {
#ifdef JOPA_DEBUG
                if (control.option.debug_trace_stack_change)
                    Coutput << "RecordFrameWithSavedLocalsAndStack: UPDATING at pc " << pc
                            << " from " << recorded_frames[i]->locals.Length()
                            << " to " << new_locals_len << " locals" << endl;
#endif
                // Clear and rebuild locals list with fewer entries
                while (recorded_frames[i]->locals.Length() > 0)
                    recorded_frames[i]->locals.Reset(recorded_frames[i]->locals.Length() - 1);
                if (saved_locals)
                {
                    for (unsigned j = 0; j < saved_locals->Length(); j++)
                        recorded_frames[i]->locals.Next() = (*saved_locals)[j];
                }
            }
#ifdef JOPA_DEBUG
            else if (control.option.debug_trace_stack_change)
            {
                Coutput << "RecordFrameWithSavedLocalsAndStack: SKIPPING at pc " << pc
                        << " (existing " << recorded_frames[i]->locals.Length()
                        << " <= new " << new_locals_len << ")" << endl;
            }
#endif
            return;
        }
    }

#ifdef JOPA_DEBUG
    if (control.option.debug_trace_stack_change)
        Coutput << "RecordFrameWithSavedLocalsAndStack: recording at pc " << pc
                << " with " << new_locals_len << " locals" << endl;
#endif

    FrameEntry* entry = new FrameEntry(pc);

    // Copy saved locals
    if (saved_locals)
    {
        for (unsigned i = 0; i < saved_locals->Length(); i++)
            entry->locals.Next() = (*saved_locals)[i];
    }

    // Copy saved stack
    if (saved_stack)
    {
        for (unsigned i = 0; i < saved_stack->Length(); i++)
            entry->stack.Next() = (*saved_stack)[i];
    }

    // Insert in sorted order by PC
    unsigned insert_pos = recorded_frames.Length();
    for (unsigned i = 0; i < recorded_frames.Length(); i++)
    {
        if (recorded_frames[i]->pc > pc)
        {
            insert_pos = i;
            break;
        }
    }

    // Make room and insert
    if (insert_pos == recorded_frames.Length())
    {
        recorded_frames.Next() = entry;
    }
    else
    {
        // Shift elements to make room
        recorded_frames.Next() = NULL; // Extend array
        for (unsigned i = recorded_frames.Length() - 1; i > insert_pos; i--)
            recorded_frames[i] = recorded_frames[i - 1];
        recorded_frames[insert_pos] = entry;
    }
}

void StackMapGenerator::RecordFrameWithSavedStackPlusInt(u2 pc, Tuple<VerificationType>* saved_stack)
{
    // Don't record duplicate frames at the same PC
    if (HasFrameAt(pc))
        return;

    FrameEntry* entry = new FrameEntry(pc);

    // Copy current locals
    for (unsigned i = 0; i < current_locals.Length(); i++)
        entry->locals.Next() = current_locals[i];

    // Copy saved stack
    if (saved_stack)
    {
        for (unsigned i = 0; i < saved_stack->Length(); i++)
            entry->stack.Next() = (*saved_stack)[i];
    }

    // Add Integer type on top
    entry->stack.Next() = VerificationType(VerificationType::TYPE_Integer);

    // Insert in sorted order by PC
    unsigned insert_pos = recorded_frames.Length();
    for (unsigned i = 0; i < recorded_frames.Length(); i++)
    {
        if (recorded_frames[i]->pc > pc)
        {
            insert_pos = i;
            break;
        }
    }

    // Make room and insert
    if (insert_pos == recorded_frames.Length())
    {
        recorded_frames.Next() = entry;
    }
    else
    {
        // Shift elements to make room
        recorded_frames.Next() = NULL; // Extend array
        for (unsigned i = recorded_frames.Length() - 1; i > insert_pos; i--)
            recorded_frames[i] = recorded_frames[i - 1];
        recorded_frames[insert_pos] = entry;
    }
}

bool StackMapGenerator::HasFrameAt(u2 pc) const
{
    for (unsigned i = 0; i < recorded_frames.Length(); i++)
    {
        if (recorded_frames[i]->pc == pc)
            return true;
    }
    return false;
}

StackMapTableAttribute* StackMapGenerator::GenerateAttribute(u2 name_index)
{
    if (recorded_frames.Length() == 0)
        return NULL;

    StackMapTableAttribute* attr = new StackMapTableAttribute(name_index);

    u2 prev_pc = 0; // Previous frame's actual bytecode offset (not delta)

    for (unsigned i = 0; i < recorded_frames.Length(); i++)
    {
        FrameEntry* entry = recorded_frames[i];

        // Calculate offset_delta
        // First frame: offset_delta = pc (offset from start)
        // Subsequent frames: offset_delta = pc - prev_pc - 1
        u2 offset_delta;
        if (i == 0)
        {
            offset_delta = entry->pc;
        }
        else
        {
            offset_delta = entry->pc - prev_pc - 1;
        }

        StackMapFrame* frame = new StackMapFrame(offset_delta);

        // Copy locals to frame, skipping implicit Top after Double/Long
        // Per JVM spec 4.7.4, Double and Long verification types implicitly
        // occupy two local variable slots, so we don't write explicit Top
        // for the second slot.
        for (unsigned j = 0; j < entry->locals.Length(); j++)
        {
            // Skip Top entries that follow Double or Long (they are implicit)
            if (j > 0 && entry->locals[j].Tag() == VerificationType::TYPE_Top)
            {
                VerificationType::VerificationTypeInfoTag prev_tag = entry->locals[j-1].Tag();
                if (prev_tag == VerificationType::TYPE_Double ||
                    prev_tag == VerificationType::TYPE_Long)
                {
                    continue; // Skip this implicit Top
                }
            }
            frame->AddLocal(entry->locals[j]);
        }

        // Copy stack to frame (same logic for stack)
        for (unsigned j = 0; j < entry->stack.Length(); j++)
        {
            // Skip Top entries that follow Double or Long
            if (j > 0 && entry->stack[j].Tag() == VerificationType::TYPE_Top)
            {
                VerificationType::VerificationTypeInfoTag prev_tag = entry->stack[j-1].Tag();
                if (prev_tag == VerificationType::TYPE_Double ||
                    prev_tag == VerificationType::TYPE_Long)
                {
                    continue;
                }
            }
            frame->AddStack(entry->stack[j]);
        }

        attr->AddFrame(frame);
        prev_pc = entry->pc;
    }

    return attr;
}

void StackMapGenerator::MarkSuperCalled()
{
    if (is_constructor && !super_called)
    {
        super_called = true;
        // Replace UninitializedThis with actual type
        if (current_locals.Length() > 0 &&
            current_locals[0].Tag() == VerificationType::TYPE_UninitializedThis)
        {
            SetLocal(0, containing_type);
        }
    }
}


} // Close namespace Jopa block
