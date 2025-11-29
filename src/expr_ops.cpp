// Semantic expression processing - operators and assignments
// Split from expr.cpp for maintainability

#include "platform.h"
#include "double.h"
#include "parser.h"
#include "semantic.h"
#include "control.h"
#include "table.h"
#include "tuple.h"
#include "spell.h"
#include "option.h"
#include "stream.h"
#include "typeparam.h"
#include "paramtype.h"

namespace Jopa {

template <typename T> inline void ExtremaForType(T& min, T& max);

template <> inline void ExtremaForType(i4& min, i4& max)
{
    min = Int::MIN_INT();
    max = Int::MAX_INT();
}

template <> inline void ExtremaForType(LongInt& min, LongInt& max)
{
    min = LongInt::MIN_LONG();
    max = LongInt::MAX_LONG();
}

inline void ReportOverflow(Semantic* semantic, AstExpression* expr, bool safe)
{
    if (! safe)
    {
        semantic -> ReportSemError(SemanticError::CONSTANT_OVERFLOW, expr,
                                   expr -> Type() -> Name());
    }
}


template <typename T>
static void CheckIntegerNegation(Semantic* semantic, AstExpression* expr,
                                 const T& x)
{
    T min, max;
    ExtremaForType(min, max);
    ReportOverflow(semantic, expr, (x != min));
}


template <typename T>
inline void CheckIntegerAddition(Semantic* semantic, AstExpression* expr,
                                 const T& x, const T& y)
{
    const T zero = T(0);
    T min, max;
    ExtremaForType(min, max);
    bool safe = x == zero ||
                y == zero ||
                (x < zero && y < zero && x >= (min - y)) ||
                (x < zero && y > zero) ||
                (x > zero && y < zero) ||
                (x > zero && y > zero && x <= (max - y));
    ReportOverflow(semantic, expr, safe);
}


template <typename T>
static void CheckIntegerSubtraction(Semantic* semantic, AstExpression* expr,
                                    const T& x, const T& y)
{
    CheckIntegerAddition(semantic, expr, x, T(-y));
}


template <typename T>
static void CheckIntegerMultiplication(Semantic* semantic, AstExpression* expr,
                                       const T& x, const T& y)
{
    const T zero = T(0);
    const T one = T(1);
    const T minus_one = T(-1);
    T min, max;
    ExtremaForType(min, max);
    bool safe = (x > minus_one && x <= one) ||
                (y > minus_one && y <= one) ||
                (x < zero && y < zero && T(-x) <= max/-y) ||
                (x < zero && y > zero && x >= min/y) ||
                (x > zero && y < zero && y >= min/x) ||
                (x > zero && y > zero && x <= max/y);
    ReportOverflow(semantic, expr, safe);
}


template <typename T>
static void CheckIntegerDivision(Semantic* semantic, AstExpression* expr,
                                 const T& x, const T& y)
{
    const T zero = T(0);
    const T minus_one = T(-1);
    T min, max;
    ExtremaForType(min, max);
    bool safe = (y != zero) && !(x == min && y == minus_one);
    ReportOverflow(semantic, expr, safe);
}



void Semantic::ProcessPostUnaryExpression(Ast* expr)
{
    AstPostUnaryExpression* postfix_expression =
        (AstPostUnaryExpression*) expr;
    AstExpression* expression = postfix_expression -> expression;

    ProcessExpression(expression);
    postfix_expression -> symbol = expression -> symbol;

    //
    // JLS2 added ability for parenthesized variable to remain a variable.
    //
    if (expression -> ParenthesizedExpressionCast())
    {
        ReportSemError(SemanticError::UNNECESSARY_PARENTHESIS, expression);
        while (expression -> ParenthesizedExpressionCast())
            expression = ((AstParenthesizedExpression*) expression) ->
                expression;
    }

    if (expression -> symbol != control.no_type)
    {
        if (! expression -> IsLeftHandSide())
        {
            ReportSemError(SemanticError::NOT_A_NUMERIC_VARIABLE,
                           postfix_expression -> expression,
                           postfix_expression -> expression -> Type() -> Name());
            postfix_expression -> symbol = control.no_type;
        }
        else if (! control.IsNumeric(expression -> Type()))
        {
            ReportSemError(SemanticError::TYPE_NOT_NUMERIC,
                           postfix_expression -> expression,
                           expression -> Type() -> ContainingPackageName(),
                           expression -> Type() -> ExternalName());
            postfix_expression -> symbol = control.no_type;
        }
        else if (! expression -> ArrayAccessCast()) // some kind of name
        {
            MethodSymbol* read_method = NULL;
            AstName* name = expression -> NameCast();
            if (name)
            {
                if (name -> resolution_opt)
                    read_method =
                        name -> resolution_opt -> symbol -> MethodCast();
            }
            else
            {
                AstFieldAccess* field_access = (AstFieldAccess*) expression;
                if (field_access -> resolution_opt)
                    read_method = field_access -> resolution_opt ->
                        symbol -> MethodCast();
            }

            if (read_method)
            {
                postfix_expression -> write_method =
                    read_method -> containing_type ->
                    GetWriteAccessFromReadAccess(read_method);
            }
        }
    }
}


void Semantic::ProcessPLUS(AstPreUnaryExpression* expr)
{
    ProcessExpression(expr -> expression);
    expr -> expression = PromoteUnaryNumericExpression(expr -> expression);
    expr -> value = expr -> expression -> value;
    expr -> symbol = expr -> expression -> symbol;
}


void Semantic::ProcessMINUS(AstPreUnaryExpression* expr)
{
    AstIntegerLiteral* int_literal =
        expr -> expression -> IntegerLiteralCast();
    AstLongLiteral* long_literal = expr -> expression -> LongLiteralCast();

    if (int_literal)
    {
        LiteralSymbol* literal = lex_stream ->
            LiteralSymbol(int_literal -> integer_literal_token);

        expr -> value = control.int_pool.FindOrInsertNegativeInt(literal);
        if (expr -> value == control.BadValue())
        {
            ReportSemError(SemanticError::INVALID_INT_VALUE, expr);
            expr -> symbol = control.no_type;
        }
        else expr -> symbol = control.int_type;
    }
    else if (long_literal)
    {
        LiteralSymbol* literal = lex_stream ->
            LiteralSymbol(long_literal -> long_literal_token);

        expr -> value = control.long_pool.FindOrInsertNegativeLong(literal);
        if (expr -> value == control.BadValue())
        {
            ReportSemError(SemanticError::INVALID_LONG_VALUE, expr);
            expr -> symbol = control.no_type;
        }
        else expr -> symbol = control.long_type;
    }
    else
    {
        ProcessExpression(expr -> expression);

        expr -> expression = PromoteUnaryNumericExpression(expr -> expression);
        expr -> symbol = expr -> expression -> symbol;
        if (expr -> expression -> IsConstant())
        {
            TypeSymbol* type = expr -> Type();

            if (type == control.double_type)
            {
                DoubleLiteralValue* literal = DYNAMIC_CAST<DoubleLiteralValue*>
                    (expr -> expression -> value);
                expr -> value =
                    control.double_pool.FindOrInsert(-literal -> value);
            }
            else if (type == control.float_type)
            {
                FloatLiteralValue* literal = DYNAMIC_CAST<FloatLiteralValue*>
                    (expr -> expression -> value);
                expr -> value =
                    control.float_pool.FindOrInsert(-literal -> value);
            }
            else if (type == control.long_type)
            {
                LongLiteralValue* literal = DYNAMIC_CAST<LongLiteralValue*>
                    (expr -> expression -> value);
                CheckIntegerNegation(this, expr, literal -> value);
                expr -> value =
                    control.long_pool.FindOrInsert(-literal -> value);
            }
            else if (expr -> Type() == control.int_type)
            {
                IntLiteralValue* literal = DYNAMIC_CAST<IntLiteralValue*>
                    (expr -> expression -> value);
                CheckIntegerNegation(this, expr, literal -> value);
                expr -> value =
                    control.int_pool.FindOrInsert(-literal -> value);
            }
        }
    }
}


void Semantic::ProcessTWIDDLE(AstPreUnaryExpression* expr)
{
    ProcessExpression(expr -> expression);
    expr -> expression = PromoteUnaryNumericExpression(expr -> expression);
    expr -> symbol = expr -> expression -> symbol;

    if (! control.IsIntegral(expr -> expression -> Type()))
    {
        TypeSymbol* type = expr -> expression -> Type();
        if (expr -> expression -> symbol != control.no_type)
            ReportSemError(SemanticError::TYPE_NOT_INTEGRAL,
                           expr -> expression,
                           type -> ContainingPackageName(),
                           type -> ExternalName());
        expr -> symbol = control.no_type;
    }
    else if (expr -> expression -> IsConstant())
    {
        if (expr -> expression -> Type() == control.long_type)
        {
            LongLiteralValue* literal = DYNAMIC_CAST<LongLiteralValue*>
                (expr -> expression -> value);
            expr -> value = control.long_pool.FindOrInsert(~literal -> value);
        }
        else // assert(expr -> expression -> Type() == control.int_type)
        {
            IntLiteralValue* literal = DYNAMIC_CAST<IntLiteralValue*>
                (expr -> expression -> value);
            expr -> value = control.int_pool.FindOrInsert(~literal -> value);
        }
    }
}


void Semantic::ProcessNOT(AstPreUnaryExpression* expr)
{
    ProcessExpression(expr -> expression);

    TypeSymbol* type = expr -> expression -> Type();

    // Java 5: Unbox Boolean to boolean
    if (type == control.Boolean())
    {
        expr -> expression = ConvertToType(expr -> expression, control.boolean_type);
        type = control.boolean_type;
    }

    if (type != control.boolean_type)
    {
        if (expr -> expression -> symbol != control.no_type)
            ReportSemError(SemanticError::TYPE_NOT_BOOLEAN,
                           expr -> expression,
                           type -> ContainingPackageName(),
                           type -> ExternalName());
        expr -> symbol = control.no_type;
    }
    else
    {
        if (expr -> expression -> IsConstant())
            expr -> value = control.int_pool
                .FindOrInsert(IsConstantTrue(expr -> expression) ? 0 : 1);
        expr -> symbol = control.boolean_type;
    }
}


void Semantic::ProcessPLUSPLUSOrMINUSMINUS(AstPreUnaryExpression* prefix_expression)
{
    AstExpression* expression = prefix_expression -> expression;

    ProcessExpression(expression);
    prefix_expression -> symbol = expression -> symbol;

    //
    // JLS2 added ability for parenthesized variable to remain a variable.
    //
    if (expression -> ParenthesizedExpressionCast())
    {
        ReportSemError(SemanticError::UNNECESSARY_PARENTHESIS, expression);
        while (expression -> ParenthesizedExpressionCast())
            expression = ((AstParenthesizedExpression*) expression) ->
                expression;
    }

    if (expression -> symbol != control.no_type)
    {
        if (! expression -> IsLeftHandSide())
        {
            ReportSemError(SemanticError::NOT_A_NUMERIC_VARIABLE,
                           prefix_expression -> expression,
                           prefix_expression -> expression -> Type() -> Name());
            prefix_expression -> symbol = control.no_type;
        }
        else if (! control.IsNumeric(expression -> Type()))
        {
            ReportSemError(SemanticError::TYPE_NOT_NUMERIC,
                           prefix_expression -> expression,
                           expression -> Type() -> ContainingPackageName(),
                           expression -> Type() -> ExternalName());
            prefix_expression -> symbol = control.no_type;
        }
        else if (! expression -> ArrayAccessCast()) // some kind of name
        {
            MethodSymbol* read_method = NULL;
            AstName* name = expression -> NameCast();
            if (name)
            {
                if (name -> resolution_opt)
                   read_method =
                       name -> resolution_opt -> symbol -> MethodCast();
            }
            else
            {
                AstFieldAccess* field_access = (AstFieldAccess*) expression;
                if (field_access -> resolution_opt)
                    read_method = field_access -> resolution_opt -> symbol ->
                        MethodCast();
            }

            if (read_method)
            {
                prefix_expression -> write_method =
                    read_method -> containing_type ->
                    GetWriteAccessFromReadAccess(read_method);
            }
        }
    }
}


void Semantic::ProcessPreUnaryExpression(Ast* expr)
{
    AstPreUnaryExpression* prefix_expression = (AstPreUnaryExpression*) expr;
    (this ->* ProcessPreUnaryExpr[prefix_expression -> Tag()])
        (prefix_expression);
}


//
// Returns true if both types are primitive, and the source type can be
// widened into the target type.
//
bool Semantic::CanWideningPrimitiveConvert(const TypeSymbol* target_type,
                                           const TypeSymbol* source_type)
{
    if (target_type == control.double_type)
        return source_type == control.float_type ||
            source_type == control.long_type ||
            source_type == control.int_type ||
            source_type == control.char_type ||
            source_type == control.short_type ||
            source_type == control.byte_type;
    if (target_type == control.float_type)
        return source_type == control.long_type ||
            source_type == control.int_type ||
            source_type == control.char_type ||
            source_type == control.short_type ||
            source_type == control.byte_type;
    if (target_type == control.long_type)
        return source_type == control.int_type ||
            source_type == control.char_type ||
            source_type == control.short_type ||
            source_type == control.byte_type;
    if (target_type == control.int_type)
        return source_type == control.char_type ||
            source_type == control.short_type ||
            source_type == control.byte_type;
    if (target_type == control.short_type)
        return source_type == control.byte_type;
    return false;
}


//
// Returns true if both types are primitive, and the source type can be
// narrowed to the target type.
//
bool Semantic::CanNarrowingPrimitiveConvert(const TypeSymbol* target_type,
                                            const TypeSymbol* source_type)
{
    if (target_type == control.byte_type)
        return source_type == control.double_type ||
            source_type == control.float_type ||
            source_type == control.long_type ||
            source_type == control.int_type ||
            source_type == control.char_type ||
            source_type == control.short_type;
    if (target_type == control.char_type)
        return source_type == control.double_type ||
            source_type == control.float_type ||
            source_type == control.long_type ||
            source_type == control.int_type ||
            source_type == control.short_type ||
            source_type == control.byte_type;
    if (target_type == control.short_type)
        return source_type == control.double_type ||
            source_type == control.float_type ||
            source_type == control.long_type ||
            source_type == control.int_type ||
            source_type == control.char_type;
    if (target_type == control.int_type)
        return source_type == control.double_type ||
            source_type == control.float_type ||
            source_type == control.long_type;
    if (target_type == control.long_type)
        return source_type == control.double_type ||
            source_type == control.float_type;
    if (target_type == control.float_type)
        return source_type == control.double_type;
    return false;
}


//
// Returns true if the source type can be converted to the target type in a
// method invocation - this includes identity and widening conversions.
//
bool Semantic::CanMethodInvocationConvert(const TypeSymbol* target_type,
                                          const TypeSymbol* source_type)
{
    if (target_type == control.no_type) // Don't convert any class to bad type.
        return false;
    if (source_type == control.no_type) // Allow bad type to match anything.
        return true;

    if (source_type -> Primitive())
    {
        if (target_type -> Primitive())
        {
            return target_type == source_type ||
                CanWideningPrimitiveConvert(target_type, source_type);
        }
        // Java 5: Boxing conversion (primitive → wrapper) for method invocation
        // JLS 5.3: Boxing conversion followed by widening reference conversion
        // e.g., int → Integer → Object
        if (control.option.source >= JopaOption::SDK1_5)
        {
            TypeSymbol* wrapper = GetWrapperType((TypeSymbol*)source_type);
            if (wrapper && wrapper -> IsSubtype(target_type))
                return true;
        }
        return false;
    }

    if (target_type -> Primitive())
    {
        // Java 5: Unboxing conversion (wrapper → primitive) for method invocation
        if (control.option.source >= JopaOption::SDK1_5 &&
            IsUnboxingConversion((TypeSymbol*)source_type, (TypeSymbol*)target_type))
            return true;
        return false;
    }
    return source_type == control.null_type ||
        source_type -> IsSubtype(target_type);
}


//
// Returns true if the reference source type can be automatically converted to
// the target type in assignments. This works only for references (including
// null), but allows a bad target type while method invocation does not.
//
bool Semantic::CanAssignmentConvertReference(const TypeSymbol* target_type,
                                             const TypeSymbol* source_type)
{
    if (target_type == control.no_type)
        return true;

    if (CanMethodInvocationConvert(target_type, source_type))
        return true;

    // Java 5: Boxing conversion (primitive → wrapper)
    if (control.option.source >= JopaOption::SDK1_5 &&
        IsBoxingConversion((TypeSymbol*)source_type, (TypeSymbol*)target_type))
        return true;

    // Java 5: Unboxing conversion (wrapper → primitive)
    if (control.option.source >= JopaOption::SDK1_5 &&
        IsUnboxingConversion((TypeSymbol*)source_type, (TypeSymbol*)target_type))
        return true;

    return false;
}


//
// Returns true if the source expression can be automatically converted to the
// target type. This includes all method invocation conversions, and
// additionally allows narrowing conversions of primitive constants.
//
bool Semantic::CanAssignmentConvert(const TypeSymbol* target_type,
                                    AstExpression* expr)
{
    if (target_type == control.no_type || expr -> symbol == control.no_type)
        return true;

    TypeSymbol* source_type = expr -> Type();

    if (CanMethodInvocationConvert(target_type, source_type))
        return true;

    if (IsIntValueRepresentableInType(expr, target_type))
        return true;

    // Java 5: Boxing conversion (primitive → wrapper)
    if (control.option.source >= JopaOption::SDK1_5 &&
        IsBoxingConversion((TypeSymbol*)source_type, (TypeSymbol*)target_type))
        return true;

    // Java 5: Unboxing conversion (wrapper → primitive)
    if (control.option.source >= JopaOption::SDK1_5 &&
        IsUnboxingConversion((TypeSymbol*)source_type, (TypeSymbol*)target_type))
        return true;

    return false;
}


//
// Returns true if the source type can be cast into the target type, via an
// identity, narrowing, or widening conversion. The lexical token is needed
// in case an error is encountered when resolving the target type.
//
bool Semantic::CanCastConvert(TypeSymbol* target_type, TypeSymbol* source_type,
                              TokenIndex tok)
{
    if (target_type == control.null_type)
        return false;
    if (source_type == target_type || source_type == control.no_type ||
        target_type == control.no_type)
    {
        return true;
    }

    if (source_type -> Primitive())
    {
        return target_type -> Primitive() &&
            (CanWideningPrimitiveConvert(target_type, source_type) ||
             CanNarrowingPrimitiveConvert(target_type, source_type));
    }

    if (target_type -> Primitive())
        return false;

    // Now that primitives are removed, check if one subtypes the other.
    if (source_type == control.null_type ||
        target_type -> IsSubtype(source_type) ||
        source_type -> IsSubtype(target_type))
    {
        return true;
    }

    // If we are left with arrays, see if the base types are compatible.
    if (source_type -> IsArray() || target_type -> IsArray())
    {
        if (source_type -> num_dimensions != target_type -> num_dimensions)
            return false;
        source_type = source_type -> base_type;
        target_type = target_type -> base_type;
        if (source_type -> Primitive() || target_type -> Primitive())
            return false;
    }

    //
    // Here, we are left with two reference types. Two classes are not
    // compatible at this point, and final classes do not implement
    // interfaces. Otherwise, a class can implement an interface (even with
    // conflicting signatures), but two interfaces must be compatible.
    //
    if (source_type -> ACC_FINAL() || target_type -> ACC_FINAL() ||
        (! source_type -> ACC_INTERFACE() && ! target_type -> ACC_INTERFACE()))
    {
         return false;
    }
    if (! source_type -> ACC_INTERFACE() || ! target_type -> ACC_INTERFACE())
        return true;
    if (! source_type -> expanded_method_table)
        ComputeMethodsClosure(source_type, tok);
    if (! target_type -> expanded_method_table)
        ComputeMethodsClosure(target_type, tok);
    ExpandedMethodTable* source_method_table =
        source_type -> expanded_method_table;
    unsigned i;
    for (i = 0; i < source_method_table -> symbol_pool.Length(); i++)
    {
        MethodSymbol* method1 =
            source_method_table -> symbol_pool[i] -> method_symbol;
        MethodShadowSymbol* method_shadow2 =
            target_type -> expanded_method_table ->
            FindOverloadMethodShadow(method1, this, tok);
        if (method_shadow2)
        {
            if (! method1 -> IsTyped())
                method1 -> ProcessMethodSignature(this, tok);

            MethodSymbol* method2 = method_shadow2 -> method_symbol;
            if (! method2 -> IsTyped())
                method2 -> ProcessMethodSignature(this, tok);
            if (method1 -> Type() != method2 -> Type())
                return false;
        }
    }
    return true; // All the methods passed the test.
}


//
// Transfer a constant value across a primitive or String cast statement,
// whether explicit or generated.
//
LiteralValue* Semantic::CastValue(const TypeSymbol* target_type,
                                  AstExpression* expr)
{
    TypeSymbol* source_type = expr -> Type();

    if (target_type == source_type || source_type == control.no_type ||
        ! expr -> IsConstant())
    {
        assert(target_type == source_type || ! expr -> value);
        return expr -> value;
    }
    if (source_type == control.String())
        return NULL; // A string cast to a supertype is not constant.

    // Java 5: Boxing/unboxing conversions are not constant expressions
    if (control.option.source >= JopaOption::SDK1_5)
    {
        if (IsBoxingConversion((TypeSymbol*)source_type, (TypeSymbol*)target_type) ||
            IsUnboxingConversion((TypeSymbol*)source_type, (TypeSymbol*)target_type))
            return NULL;
    }

    // Reference type casts (other than String) are not constant expressions
    // This includes Object -> Logger, etc.
    if (! control.IsNumeric((TypeSymbol*)target_type) &&
        target_type != control.boolean_type &&
        target_type != control.String())
    {
        return NULL;
    }

    LiteralValue* literal_value = NULL;
    if (target_type == control.String())
    {
        if (source_type == control.double_type)
        {
            DoubleLiteralValue* literal =
                DYNAMIC_CAST<DoubleLiteralValue*> (expr -> value);
            DoubleToString ieee_double(literal -> value);
            literal_value =
                control.Utf8_pool.FindOrInsert(ieee_double.String(),
                                               ieee_double.Length());
        }
        else if (source_type == control.float_type)
        {
            FloatLiteralValue* literal =
                DYNAMIC_CAST<FloatLiteralValue*> (expr -> value);
            FloatToString ieee_float(literal -> value);
            literal_value =
                control.Utf8_pool.FindOrInsert(ieee_float.String(),
                                               ieee_float.Length());
        }
        else if (source_type == control.long_type)
        {
            LongLiteralValue* literal =
                DYNAMIC_CAST<LongLiteralValue*> (expr -> value);
            LongToString long_integer(literal -> value);
            literal_value =
                control.Utf8_pool.FindOrInsert(long_integer.String(),
                                               long_integer.Length());
        }
        else if (source_type == control.char_type)
        {
            IntLiteralValue* literal =
                DYNAMIC_CAST<IntLiteralValue*> (expr -> value);
            literal_value = control.Utf8_pool.FindOrInsert(literal -> value);
        }
        else if (source_type == control.boolean_type)
        {
            if (IsConstantFalse(expr))
                literal_value = control.false_name_symbol -> Utf8_literal;
            else
            {
                assert(IsConstantTrue(expr));
                literal_value = control.true_name_symbol -> Utf8_literal;
            }
        }
        else if (control.IsSimpleIntegerValueType(source_type))
        {
            IntLiteralValue* literal =
                DYNAMIC_CAST<IntLiteralValue*> (expr -> value);
            IntToString integer(literal -> value);
            literal_value =
                control.Utf8_pool.FindOrInsert(integer.String(),
                                               integer.Length());
        }
    }
    else if (target_type == control.double_type)
    {
        if (source_type == control.float_type)
        {
            FloatLiteralValue* literal =
                DYNAMIC_CAST<FloatLiteralValue*> (expr -> value);
            literal_value =
                control.double_pool.FindOrInsert(literal -> value.DoubleValue());
        }
        else if (source_type == control.long_type)
        {
            LongLiteralValue* literal =
                DYNAMIC_CAST<LongLiteralValue*> (expr -> value);
            IEEEdouble value(literal -> value);
            literal_value = control.double_pool.FindOrInsert(value);
        }
        else
        {
            IntLiteralValue* literal =
                DYNAMIC_CAST<IntLiteralValue*> (expr -> value);
            IEEEdouble value(literal -> value);
            literal_value = control.double_pool.FindOrInsert(value);
        }
    }
    else if (target_type == control.float_type)
    {
        if (source_type == control.double_type)
        {
            DoubleLiteralValue* literal =
                DYNAMIC_CAST<DoubleLiteralValue*> (expr -> value);
            literal_value =
                control.float_pool.FindOrInsert(literal -> value.FloatValue());
        }
        else if (source_type == control.long_type)
        {
            LongLiteralValue* literal =
                DYNAMIC_CAST<LongLiteralValue*> (expr -> value);
            IEEEfloat value(literal -> value);
            literal_value = control.float_pool.FindOrInsert(value);
        }
        else
        {
            IntLiteralValue* literal =
                DYNAMIC_CAST<IntLiteralValue*> (expr -> value);
            IEEEfloat value(literal -> value);
            literal_value = control.float_pool.FindOrInsert(value);
        }
    }
    else if (target_type == control.long_type)
    {
        if (source_type == control.double_type)
        {
            DoubleLiteralValue* literal =
                DYNAMIC_CAST<DoubleLiteralValue*> (expr -> value);
            literal_value =
                control.long_pool.FindOrInsert(literal -> value.LongValue());
        }
        else if (source_type == control.float_type)
        {
            FloatLiteralValue* literal =
                DYNAMIC_CAST<FloatLiteralValue*> (expr -> value);
            literal_value =
                control.long_pool.FindOrInsert(literal -> value.LongValue());
        }
        else
        {
            IntLiteralValue* literal =
                DYNAMIC_CAST<IntLiteralValue*> (expr -> value);
            literal_value =
                control.long_pool.FindOrInsert((LongInt) literal -> value);
        }
    }
    else if (target_type == control.int_type)
    {
        if (source_type == control.double_type)
        {
            DoubleLiteralValue* literal =
                DYNAMIC_CAST<DoubleLiteralValue*> (expr -> value);
            literal_value =
                control.int_pool.FindOrInsert((literal -> value).IntValue());
        }
        else if (source_type == control.float_type)
        {
            FloatLiteralValue* literal =
                DYNAMIC_CAST<FloatLiteralValue*> (expr -> value);
            literal_value =
                control.int_pool.FindOrInsert(literal -> value.IntValue());
        }
        else if (source_type == control.long_type)
        {
            LongLiteralValue* literal =
                DYNAMIC_CAST<LongLiteralValue*> (expr -> value);
            literal_value =
                control.int_pool.FindOrInsert((i4) (literal -> value).LowWord());
        }
        else literal_value = expr -> value;
    }
    else if (target_type == control.char_type)
    {
        if (source_type == control.double_type)
        {
            DoubleLiteralValue* literal =
                DYNAMIC_CAST<DoubleLiteralValue*> (expr -> value);
            literal_value =
                control.int_pool.FindOrInsert((i4) (u2) (literal -> value.IntValue()));
        }
        else if (source_type == control.float_type)
        {
            FloatLiteralValue* literal =
                DYNAMIC_CAST<FloatLiteralValue*> (expr -> value);
            literal_value =
                control.int_pool.FindOrInsert((i4) (u2) (literal -> value.IntValue()));
        }
        else if (source_type == control.long_type)
        {
            LongLiteralValue* literal =
                DYNAMIC_CAST<LongLiteralValue*> (expr -> value);
            literal_value =
                control.int_pool.FindOrInsert((i4) (u2) (literal -> value).LowWord());
        }
        else
        {
            IntLiteralValue* literal =
                DYNAMIC_CAST<IntLiteralValue*> (expr -> value);
            literal_value =
                control.int_pool.FindOrInsert((i4) (u2) literal -> value);
        }
    }
    else if (target_type == control.short_type)
    {
        if (source_type == control.double_type)
        {
            DoubleLiteralValue* literal =
                DYNAMIC_CAST<DoubleLiteralValue*> (expr -> value);
            literal_value =
                control.int_pool.FindOrInsert((i4) (i2) (literal -> value.IntValue()));
        }
        else if (source_type == control.float_type)
        {
            FloatLiteralValue* literal =
                DYNAMIC_CAST<FloatLiteralValue*> (expr -> value);
            literal_value =
                control.int_pool.FindOrInsert((i4) (i2) (literal -> value.IntValue()));
        }
        else if (source_type == control.long_type)
        {
            LongLiteralValue* literal =
                DYNAMIC_CAST<LongLiteralValue*> (expr -> value);
            literal_value =
                control.int_pool.FindOrInsert((i4) (i2) (literal -> value).LowWord());
        }
        else
        {
            IntLiteralValue* literal =
                DYNAMIC_CAST<IntLiteralValue*> (expr -> value);
            literal_value =
                control.int_pool.FindOrInsert((i4) (i2) literal -> value);
        }
    }
    else if (target_type == control.byte_type)
    {
        if (source_type == control.double_type)
        {
            DoubleLiteralValue* literal =
                DYNAMIC_CAST<DoubleLiteralValue*> (expr -> value);
            literal_value =
                control.int_pool.FindOrInsert((i4) (i1) (literal -> value.IntValue()));
        }
        else if (source_type == control.float_type)
        {
            FloatLiteralValue* literal =
                DYNAMIC_CAST<FloatLiteralValue*> (expr -> value);
            literal_value =
                control.int_pool.FindOrInsert((i4) (i1) (literal -> value.IntValue()));
        }
        else if (source_type == control.long_type)
        {
            LongLiteralValue* literal =
                DYNAMIC_CAST<LongLiteralValue*> (expr -> value);
            literal_value =
                control.int_pool.FindOrInsert((i4) (i1)
                                              (literal -> value).LowWord());
        }
        else
        {
            IntLiteralValue* literal =
                DYNAMIC_CAST<IntLiteralValue*> (expr -> value);
            literal_value =
                control.int_pool.FindOrInsert((i4) (i1) literal -> value);
        }
    }

    assert(literal_value);
    return literal_value;
}


void Semantic::ProcessCastExpression(Ast* expr)
{
    AstCastExpression* cast_expression = (AstCastExpression*) expr;

    //
    // Do not use ProcessExpressionOrStringConstant here, to avoid generating
    // intermediate Strings - see CheckConstantString in lookup.cpp
    //
    ProcessType(cast_expression -> type);
    ProcessExpression(cast_expression -> expression);

    TypeSymbol* source_type = cast_expression -> expression -> Type();
    TypeSymbol* target_type = cast_expression -> type -> symbol;

    if (CanCastConvert(target_type, source_type,
                       cast_expression -> right_parenthesis_token))
    {
        cast_expression -> symbol = target_type;
        cast_expression -> value = CastValue(target_type,
                                             cast_expression -> expression);

        // Check for unchecked casts to generic types
        // Unchecked casts occur when casting to a parameterized type,
        // because type arguments cannot be verified at runtime due to erasure
        AstTypeName* target_name = cast_expression -> type -> TypeNameCast();
        if (target_name && target_name -> type_arguments_opt)
        {
            // Casting to a parameterized type - this is unchecked
            // Example: (List<String>) obj
            if (control.option.source >= JopaOption::SDK1_5 &&
                ! control.option.nowarn_unchecked)
            {
                ReportSemError(SemanticError::UNCHECKED_TYPE_CONVERSION,
                              cast_expression,
                              source_type -> ContainingPackageName(),
                              source_type -> ExternalName(),
                              target_type -> ContainingPackageName(),
                              target_type -> ExternalName());
            }
        }
        else if (target_type -> IsGeneric() && !target_name)
        {
            // Casting to raw generic type from non-generic source
            // Example: (List) obj where List is generic
            if (control.option.source >= JopaOption::SDK1_5 &&
                ! control.option.nowarn_unchecked)
            {
                ReportSemError(SemanticError::UNCHECKED_TYPE_CONVERSION,
                              cast_expression,
                              source_type -> ContainingPackageName(),
                              source_type -> ExternalName(),
                              target_type -> ContainingPackageName(),
                              target_type -> ExternalName());
            }
        }
    }
    else
    {
        ReportSemError(SemanticError::INVALID_CAST_CONVERSION,
                       cast_expression -> expression,
                       source_type -> ContainingPackageName(),
                       source_type -> Name(),
                       target_type -> ContainingPackageName(),
                       target_type -> Name());
        cast_expression -> symbol = control.no_type;
    }
}


//
// Inserts a widening conversion, if necessary.
//
AstExpression* Semantic::ConvertToType(AstExpression* expr,
                                       TypeSymbol* target_type)
{
    TypeSymbol* source_type = expr -> Type();
    if (source_type == control.null_type || source_type == target_type ||
        source_type == control.no_type || target_type -> Bad())
    {
        return expr;
    }

    TokenIndex loc = expr -> LeftToken();

    AstCastExpression* result =
        compilation_unit -> ast_pool -> GenCastExpression();
    result -> left_parenthesis_token = loc;
    //
    // Rather than generate an AstType, we leave this NULL and rely
    // on the resolved symbol for the type.
    //
    result -> type = NULL;
    result -> right_parenthesis_token = loc;
    result -> expression = expr;
    result -> symbol = target_type;
    result -> value = CastValue(target_type, expr);
    return result;
}


AstExpression* Semantic::PromoteUnaryNumericExpression(AstExpression* unary_expression)
{
    TypeSymbol* type = unary_expression -> Type();

    if (type == control.no_type)
        return unary_expression;

    // Java 5: Unbox wrapper types to primitives before numeric promotion
    if (control.option.source >= JopaOption::SDK1_5)
    {
        TypeSymbol* unboxed_type = type -> UnboxedType(control);
        if (unboxed_type && unboxed_type != type)
        {
            unary_expression = ConvertToType(unary_expression, unboxed_type);
            type = unboxed_type;
        }
    }

    if (! control.IsNumeric(type))
    {
        ReportSemError(SemanticError::TYPE_NOT_NUMERIC, unary_expression,
                       type -> ContainingPackageName(),
                       type -> ExternalName());
        unary_expression -> symbol = control.no_type;
        return unary_expression;
    }
    return (type == control.byte_type || type == control.short_type ||
            type == control.char_type)
        ? ConvertToType(unary_expression, control.int_type) : unary_expression;
}


void Semantic::BinaryNumericPromotion(AstBinaryExpression* binary_expression)
{
    binary_expression -> symbol =
        BinaryNumericPromotion(binary_expression -> left_expression,
                               binary_expression -> right_expression);
}


void Semantic::BinaryNumericPromotion(AstAssignmentExpression* assignment_expression)
{
    AstExpression* left_expr = assignment_expression -> left_hand_side;
    while (left_expr -> ParenthesizedExpressionCast())
        left_expr = ((AstParenthesizedExpression*) left_expr) -> expression;
    TypeSymbol* type =
        BinaryNumericPromotion(left_expr, assignment_expression -> expression);
    assignment_expression -> left_hand_side = left_expr;
    if (type == control.no_type)
        assignment_expression -> symbol = control.no_type;
}


void Semantic::BinaryNumericPromotion(AstConditionalExpression* conditional_expression)
{
    conditional_expression -> symbol =
        BinaryNumericPromotion(conditional_expression -> true_expression,
                               conditional_expression -> false_expression);
}


TypeSymbol* Semantic::BinaryNumericPromotion(AstExpression*& left_expr,
                                             AstExpression*& right_expr)
{
    TypeSymbol* left_type = left_expr -> Type();
    TypeSymbol* right_type = right_expr -> Type();

    // Java 5: Unbox wrapper types to primitives before numeric promotion
    if (control.option.source >= JopaOption::SDK1_5)
    {
        TypeSymbol* unboxed_left = left_type -> UnboxedType(control);
        if (unboxed_left && unboxed_left != left_type)
        {
            left_expr = ConvertToType(left_expr, unboxed_left);
            left_type = unboxed_left;
        }

        TypeSymbol* unboxed_right = right_type -> UnboxedType(control);
        if (unboxed_right && unboxed_right != right_type)
        {
            right_expr = ConvertToType(right_expr, unboxed_right);
            right_type = unboxed_right;
        }
    }

    if (! control.IsNumeric(left_type) || ! control.IsNumeric(right_type))
    {
        if (left_type != control.no_type && ! control.IsNumeric(left_type))
            ReportSemError(SemanticError::TYPE_NOT_NUMERIC, left_expr,
                           left_type -> ContainingPackageName(),
                           left_type -> ExternalName());
        if (right_type != control.no_type && ! control.IsNumeric(right_type))
            ReportSemError(SemanticError::TYPE_NOT_NUMERIC, right_expr,
                           right_type -> ContainingPackageName(),
                           right_type -> ExternalName());
        return control.no_type;
    }
    if (left_type == control.double_type)
    {
        right_expr = ConvertToType(right_expr, control.double_type);
        return control.double_type;
    }
    if (right_type == control.double_type)
    {
        left_expr = ConvertToType(left_expr, control.double_type);
        return control.double_type;
    }
    if (left_type == control.float_type)
    {
        right_expr = ConvertToType(right_expr, control.float_type);
        return control.float_type;
    }
    if (right_type == control.float_type)
    {
        left_expr = ConvertToType(left_expr, control.float_type);
        return control.float_type;
    }
    if (left_type == control.long_type)
    {
        right_expr = ConvertToType(right_expr, control.long_type);
        return control.long_type;
    }
    if (right_type == control.long_type)
    {
        left_expr = ConvertToType(left_expr, control.long_type);
        return control.long_type;
    }
    left_expr = ConvertToType(left_expr, control.int_type);
    right_expr = ConvertToType(right_expr, control.int_type);
    return control.int_type;
}


void Semantic::MethodInvocationConversion(AstArguments* args,
                                          MethodSymbol* method)
{
    bool is_varargs = method -> ACC_VARARGS();
    unsigned num_formals = method -> NumFormalParameters();
    unsigned num_args = args -> NumArguments();

    if (! is_varargs)
    {
        assert(num_args == num_formals);
        for (unsigned i = 0; i < num_args; i++)
        {
            AstExpression* expr = args -> Argument(i);
            if (expr -> Type() != method -> FormalParameter(i) -> Type())
            {
                args -> Argument(i) =
                    ConvertToType(expr, method -> FormalParameter(i) -> Type());
            }
        }
        return;
    }

    // Varargs method - need to handle variable arguments
    assert(num_formals > 0);
    TypeSymbol* varargs_type = method -> FormalParameter(num_formals - 1) -> Type();
    assert(varargs_type -> IsArray());
    TypeSymbol* component_type = varargs_type -> ArraySubtype();

    // Convert non-varargs parameters normally
    unsigned num_fixed = num_formals - 1;
    for (unsigned i = 0; i < num_fixed && i < num_args; i++)
    {
        AstExpression* expr = args -> Argument(i);
        if (expr -> Type() != method -> FormalParameter(i) -> Type())
        {
            args -> Argument(i) =
                ConvertToType(expr, method -> FormalParameter(i) -> Type());
        }
    }

    // Handle varargs parameter
    if (num_args == num_formals)
    {
        // Exact match - last argument might be an array or single element
        AstExpression* last_arg = args -> Argument(num_args - 1);
        if (CanMethodInvocationConvert(varargs_type, last_arg -> Type()))
        {
            // Already an array compatible type (same type or subtype like String[] -> Object[])
            // No wrapping needed - pass array as-is
            return;
        }
        // Fall through to wrap single element in array
    }

    // Need to wrap varargs arguments in an array
    // Create: new ComponentType[] { arg1, arg2, ... }
    StoragePool* ast_pool = compilation_unit -> ast_pool;
    TokenIndex loc = args -> left_parenthesis_token;

    AstArrayCreationExpression* array_creation = ast_pool -> GenArrayCreationExpression();
    array_creation -> new_token = loc;

    // Create array type
    AstName* component_name = ast_pool -> GenName(loc);
    component_name -> symbol = component_type;
    AstTypeName* type_name = ast_pool -> GenTypeName(component_name);
    AstBrackets* brackets = ast_pool -> GenBrackets(loc, loc);
    brackets -> dims = 1;
    AstArrayType* array_type = ast_pool -> GenArrayType(type_name, brackets);
    array_type -> symbol = varargs_type;
    array_creation -> array_type = array_type;

    // Create array initializer with varargs arguments
    unsigned num_varargs = (num_args >= num_fixed) ? (num_args - num_fixed) : 0;
    AstArrayInitializer* initializer = ast_pool -> GenArrayInitializer();
    initializer -> left_brace_token = loc;
    initializer -> right_brace_token = loc;
    initializer -> AllocateVariableInitializers(num_varargs);

    for (unsigned i = num_fixed; i < num_args; i++)
    {
        AstExpression* vararg = args -> Argument(i);
        // Convert to component type if needed
        if (vararg -> Type() != component_type)
            vararg = ConvertToType(vararg, component_type);
        initializer -> AddVariableInitializer(vararg);
    }

    array_creation -> array_initializer_opt = initializer;
    array_creation -> symbol = varargs_type;

    // Replace varargs arguments with the single array expression
    // Note: We can't resize the arguments array, so we replace in place
    // The extra arguments will remain but the method signature expects only num_formals
    // Special case: if num_args < num_formals, we can't modify arguments here
    // The bytecode generator will handle this case
    if (num_args >= num_formals)
    {
        args -> Argument(num_fixed) = array_creation;
    }
    // else: bytecode generator will create empty array when needed
}


void Semantic::ProcessPLUS(AstBinaryExpression* expr)
{
    //
    // Do not use ProcessExpressionOrStringConstant here, to avoid generating
    // intermediate Strings - see CheckConstantString in lookup.cpp
    //
    AstExpression* left = expr -> left_expression;
    AstExpression* right = expr -> right_expression;
    ProcessExpression(left);
    ProcessExpression(right);

    TypeSymbol* left_type = left -> Type();
    TypeSymbol* right_type = right -> Type();

    if (left_type == control.no_type || right_type == control.no_type)
        expr -> symbol = control.no_type;
    else if (left_type == control.String() || right_type == control.String())
    {
        //
        // Convert the left expression if necessary.
        //
        if (left_type != control.String())
        {
            AddDependence(ThisType(), left_type -> BoxedType(control));
            if (left_type == control.void_type)
            {
                ReportSemError(SemanticError::VOID_TO_STRING, left);
                expr -> symbol = control.no_type;
            }
            else if (left_type == control.null_type || left -> IsConstant())
            {
                left -> value = CastValue(control.String(), left);
                left -> symbol = control.String();
            }
        }

        //
        // Convert the right expression if necessary.
        //
        if (right_type != control.String())
        {
            AddDependence(ThisType(), right_type -> BoxedType(control));
            if (right_type == control.void_type)
            {
                ReportSemError(SemanticError::VOID_TO_STRING, right);
                expr -> symbol = control.no_type;
            }
            else if (right_type == control.null_type || right -> IsConstant())
            {
                right -> value = CastValue(control.String(), right);
                right -> symbol = control.String();
            }
        }

        AddDependence(ThisType(), control.option.target >= JopaOption::SDK1_5
                      ? control.StringBuilder() : control.StringBuffer());

        //
        // If both subexpressions are string constants, identify the result as
        // as a string constant, but do not perform the concatenation here. The
        // reason being that if we have a long expression of the form
        //
        //  s1 + s2 + ... + sn
        //
        // where each subexpression s(i) is a string constant, we want to
        // perform one concatenation and enter a single result into the
        // constant pool instead of n-1 subresults. See CheckStringConstant
        // in lookup.cpp.
        //
        if (expr -> symbol != control.no_type)
            expr -> symbol = control.String();
    }
    else
    {
        BinaryNumericPromotion(expr);
        left = expr -> left_expression;
        right = expr -> right_expression;

        if (left -> IsConstant() && right -> IsConstant())
        {
            if (expr -> Type() == control.double_type)
            {
                DoubleLiteralValue* left_value =
                    DYNAMIC_CAST<DoubleLiteralValue*> (left -> value);
                DoubleLiteralValue* right_value =
                    DYNAMIC_CAST<DoubleLiteralValue*> (right -> value);

                expr -> value =
                    control.double_pool.FindOrInsert(left_value -> value +
                                                     right_value -> value);
            }
            else if (expr -> Type() == control.float_type)
            {
                FloatLiteralValue* left_value =
                    DYNAMIC_CAST<FloatLiteralValue*> (left -> value);
                FloatLiteralValue* right_value =
                    DYNAMIC_CAST<FloatLiteralValue*> (right -> value);
                expr -> value =
                    control.float_pool.FindOrInsert(left_value -> value +
                                                    right_value -> value);
            }
            else if (expr -> Type() == control.long_type)
            {
                LongLiteralValue* left_value =
                    DYNAMIC_CAST<LongLiteralValue*> (left -> value);
                LongLiteralValue* right_value =
                    DYNAMIC_CAST<LongLiteralValue*> (right -> value);

                CheckIntegerAddition(this, expr, left_value -> value,
                                     right_value -> value);
                expr -> value =
                    control.long_pool.FindOrInsert(left_value -> value +
                                                   right_value -> value);
            }
            else if (expr -> Type() == control.int_type)
            {
                IntLiteralValue* left_value =
                    DYNAMIC_CAST<IntLiteralValue*> (left -> value);
                IntLiteralValue* right_value =
                    DYNAMIC_CAST<IntLiteralValue*> (right -> value);
                CheckIntegerAddition(this, expr, left_value -> value,
                                     right_value -> value);
                expr -> value =
                    control.int_pool.FindOrInsert(left_value -> value +
                                                  right_value -> value);
            }
        }
    }
}


void Semantic::ProcessShift(AstBinaryExpression* expr)
{
    ProcessExpression(expr -> left_expression);
    ProcessExpression(expr -> right_expression);

    TypeSymbol* left_type = expr -> left_expression -> Type();
    TypeSymbol* right_type = expr -> right_expression -> Type();

    if (! control.IsIntegral(left_type))
    {
        if (left_type != control.no_type)
            ReportSemError(SemanticError::TYPE_NOT_INTEGRAL,
                           expr -> left_expression,
                           left_type -> ContainingPackageName(),
                           left_type -> ExternalName());
        expr -> symbol = control.no_type;
    }
    else
    {
        expr -> left_expression =
            PromoteUnaryNumericExpression(expr -> left_expression);
    }
    //
    // This call captures both unary numeric conversion (widening) of
    // byte, char, or short, and narrowing of long, since the bytecode
    // requires an int shift amount.
    //
    if (! control.IsIntegral(right_type))
    {
        if (right_type != control.no_type)
            ReportSemError(SemanticError::TYPE_NOT_INTEGRAL,
                           expr -> right_expression,
                           right_type -> ContainingPackageName(),
                           right_type -> ExternalName());
        expr -> symbol = control.no_type;
    }
    else
    {
        expr -> right_expression = ConvertToType(expr -> right_expression,
                                                 control.int_type);
        if (expr -> symbol != control.no_type)
            expr -> symbol = expr -> left_expression -> symbol;

        ProcessShiftCount(left_type, expr -> right_expression);
    }
}


//
// Checks whether 'expr' is a suitable shift count for something of type
// 'left_type'. JLS2 15.19 is quite clear about the meaning of code with
// with a negative or out-of-range shift count, so it's still valid code,
// but the behavior is probably not what the author was expecting.
//
void Semantic::ProcessShiftCount(TypeSymbol* left_type, AstExpression* expr)
{
    if (! expr -> IsConstant())
        return;

    IntLiteralValue* literal = DYNAMIC_CAST<IntLiteralValue*>(expr -> value);
    i4 count = literal -> value;
    IntToWstring count_text(count);

    if (count < 0)
    {
        ReportSemError(SemanticError::NEGATIVE_SHIFT_COUNT,
                       expr,
                       count_text.String());
    }

    int width = (left_type == control.long_type) ? 64 : 32;
    if (count >= width)
    {
        IntToWstring width_text(width);
        ReportSemError(SemanticError::SHIFT_COUNT_TOO_LARGE,
                       expr,
                       count_text.String(),
                       width_text.String());
    }
}


void Semantic::ProcessLEFT_SHIFT(AstBinaryExpression* expr)
{
    ProcessShift(expr);

    if (expr -> left_expression -> IsConstant() &&
        expr -> right_expression -> IsConstant())
    {
        if (expr -> Type() == control.long_type)
        {
            LongLiteralValue* left = DYNAMIC_CAST<LongLiteralValue*>
                (expr -> left_expression -> value);
            IntLiteralValue* right = DYNAMIC_CAST<IntLiteralValue*>
                (expr -> right_expression -> value);

            expr -> value = control.long_pool.FindOrInsert(left -> value <<
                                                           (right -> value &
                                                            LONG_SHIFT_MASK));
        }
        else if (expr -> Type() == control.int_type)
        {
            IntLiteralValue* left = DYNAMIC_CAST<IntLiteralValue*>
                (expr -> left_expression -> value);
            IntLiteralValue* right = DYNAMIC_CAST<IntLiteralValue*>
                (expr -> right_expression -> value);

            expr -> value = control.int_pool.FindOrInsert(left -> value <<
                                                          (right -> value &
                                                           INT_SHIFT_MASK));
        }
    }
}


void Semantic::ProcessRIGHT_SHIFT(AstBinaryExpression* expr)
{
    ProcessShift(expr);

    if (expr -> left_expression -> IsConstant() &&
        expr -> right_expression -> IsConstant())
    {
        if (expr -> Type() == control.long_type)
        {
            LongLiteralValue* left = DYNAMIC_CAST<LongLiteralValue*>
                (expr -> left_expression -> value);
            IntLiteralValue* right = DYNAMIC_CAST<IntLiteralValue*>
                (expr -> right_expression -> value);

            expr -> value = control.long_pool.FindOrInsert(left -> value >>
                                                           (right -> value &
                                                            LONG_SHIFT_MASK));
        }
        else if (expr -> Type() == control.int_type)
        {
            IntLiteralValue* left = DYNAMIC_CAST<IntLiteralValue*>
                (expr -> left_expression -> value);
            IntLiteralValue* right = DYNAMIC_CAST<IntLiteralValue*>
                (expr -> right_expression -> value);

            expr -> value = control.int_pool.FindOrInsert(left -> value >>
                                                          (right -> value &
                                                           INT_SHIFT_MASK));
        }
    }
}


void Semantic::ProcessUNSIGNED_RIGHT_SHIFT(AstBinaryExpression* expr)
{
    ProcessShift(expr);

    if (expr -> left_expression -> IsConstant() &&
        expr -> right_expression -> IsConstant())
    {
        if (expr -> Type() == control.long_type)
        {
            LongLiteralValue* left = DYNAMIC_CAST<LongLiteralValue*>
                (expr -> left_expression -> value);
            IntLiteralValue* right = DYNAMIC_CAST<IntLiteralValue*>
                (expr -> right_expression -> value);

            expr -> value = control.long_pool.FindOrInsert((LongInt)
                ((ULongInt) left -> value >> (right -> value & LONG_SHIFT_MASK)));
        }
        else if (expr -> Type() == control.int_type)
        {
            IntLiteralValue* left = DYNAMIC_CAST<IntLiteralValue*>
                (expr -> left_expression -> value);
            IntLiteralValue* right = DYNAMIC_CAST<IntLiteralValue*>
                (expr -> right_expression -> value);

            expr -> value = control.int_pool.FindOrInsert((i4)
                ((u4) left -> value >> (right -> value & INT_SHIFT_MASK)));
        }
    }
}


void Semantic::ProcessLESS(AstBinaryExpression* expr)
{
    ProcessExpression(expr -> left_expression);
    ProcessExpression(expr -> right_expression);

    BinaryNumericPromotion(expr);
    TypeSymbol* left_type = expr -> left_expression -> Type();
    TypeSymbol* right_type = expr -> right_expression -> Type();

    expr -> symbol = (left_type == control.no_type ||
                      right_type == control.no_type)
        ? control.no_type : control.boolean_type;

    if (expr -> left_expression -> IsConstant() &&
        expr -> right_expression -> IsConstant())
    {
        if (left_type == control.double_type)
        {
            DoubleLiteralValue* left = DYNAMIC_CAST<DoubleLiteralValue*>
                (expr -> left_expression -> value);
            DoubleLiteralValue* right = DYNAMIC_CAST<DoubleLiteralValue*>
                (expr -> right_expression -> value);

            expr -> value =
                control.int_pool.FindOrInsert(left -> value <
                                              right -> value ? 1 : 0);
        }
        else if (left_type == control.float_type)
        {
            FloatLiteralValue* left = DYNAMIC_CAST<FloatLiteralValue*>
                (expr -> left_expression -> value);
            FloatLiteralValue* right = DYNAMIC_CAST<FloatLiteralValue*>
                (expr -> right_expression -> value);

            expr -> value =
                control.int_pool.FindOrInsert(left -> value <
                                              right -> value ? 1 : 0);
        }
        else if (left_type == control.long_type)
        {
            LongLiteralValue* left = DYNAMIC_CAST<LongLiteralValue*>
                (expr -> left_expression -> value);
            LongLiteralValue* right = DYNAMIC_CAST<LongLiteralValue*>
                (expr -> right_expression -> value);

            expr -> value =
                control.int_pool.FindOrInsert(left -> value <
                                              right -> value ? 1 : 0);
        }
        else if (left_type == control.int_type)
        {
            IntLiteralValue* left = DYNAMIC_CAST<IntLiteralValue*>
                (expr -> left_expression -> value);
            IntLiteralValue* right = DYNAMIC_CAST<IntLiteralValue*>
                (expr -> right_expression -> value);

            expr -> value =
                control.int_pool.FindOrInsert(left -> value <
                                              right -> value ? 1 : 0);
        }
    }
}


void Semantic::ProcessGREATER(AstBinaryExpression* expr)
{
    ProcessExpression(expr -> left_expression);
    ProcessExpression(expr -> right_expression);

    BinaryNumericPromotion(expr);
    TypeSymbol* left_type = expr -> left_expression -> Type();
    TypeSymbol* right_type = expr -> right_expression -> Type();

    expr -> symbol = (left_type == control.no_type ||
                      right_type == control.no_type)
        ? control.no_type : control.boolean_type;

    if (expr -> left_expression -> IsConstant() &&
        expr -> right_expression -> IsConstant())
    {
        if (left_type == control.double_type)
        {
            DoubleLiteralValue* left = DYNAMIC_CAST<DoubleLiteralValue*>
                (expr -> left_expression -> value);
            DoubleLiteralValue* right = DYNAMIC_CAST<DoubleLiteralValue*>
                (expr -> right_expression -> value);

            expr -> value =
                control.int_pool.FindOrInsert(left -> value >
                                              right -> value ? 1 : 0);
        }
        else if (left_type == control.float_type)
        {
            FloatLiteralValue* left = DYNAMIC_CAST<FloatLiteralValue*>
                (expr -> left_expression -> value);
            FloatLiteralValue* right = DYNAMIC_CAST<FloatLiteralValue*>
                (expr -> right_expression -> value);

            expr -> value =
                control.int_pool.FindOrInsert(left -> value >
                                              right -> value ? 1 : 0);
        }
        else if (left_type == control.long_type)
        {
            LongLiteralValue* left = DYNAMIC_CAST<LongLiteralValue*>
                (expr -> left_expression -> value);
            LongLiteralValue* right = DYNAMIC_CAST<LongLiteralValue*>
                (expr -> right_expression -> value);

            expr -> value =
                control.int_pool.FindOrInsert(left -> value >
                                              right -> value ? 1 : 0);
        }
        else if (left_type == control.int_type)
        {
            IntLiteralValue* left = DYNAMIC_CAST<IntLiteralValue*>
                (expr -> left_expression -> value);
            IntLiteralValue* right = DYNAMIC_CAST<IntLiteralValue*>
                (expr -> right_expression -> value);

            expr -> value =
                control.int_pool.FindOrInsert(left -> value >
                                              right -> value ? 1 : 0);
        }
    }
}


void Semantic::ProcessLESS_EQUAL(AstBinaryExpression* expr)
{
    ProcessExpression(expr -> left_expression);
    ProcessExpression(expr -> right_expression);

    BinaryNumericPromotion(expr);
    TypeSymbol* left_type = expr -> left_expression -> Type();
    TypeSymbol* right_type = expr -> right_expression -> Type();

    expr -> symbol = (left_type == control.no_type ||
                      right_type == control.no_type)
        ? control.no_type : control.boolean_type;

    if (expr -> left_expression -> IsConstant() &&
        expr -> right_expression -> IsConstant())
    {
        if (left_type == control.double_type)
        {
            DoubleLiteralValue* left = DYNAMIC_CAST<DoubleLiteralValue*>
                (expr -> left_expression -> value);
            DoubleLiteralValue* right = DYNAMIC_CAST<DoubleLiteralValue*>
                (expr -> right_expression -> value);

            expr -> value =
                control.int_pool.FindOrInsert(left -> value <=
                                              right -> value ? 1 : 0);
        }
        else if (left_type == control.float_type)
        {
            FloatLiteralValue* left = DYNAMIC_CAST<FloatLiteralValue*>
                (expr -> left_expression -> value);
            FloatLiteralValue* right = DYNAMIC_CAST<FloatLiteralValue*>
                (expr -> right_expression -> value);

            expr -> value =
                control.int_pool.FindOrInsert(left -> value <=
                                              right -> value ? 1 : 0);
        }
        else if (left_type == control.long_type)
        {
            LongLiteralValue* left = DYNAMIC_CAST<LongLiteralValue*>
                (expr -> left_expression -> value);
            LongLiteralValue* right = DYNAMIC_CAST<LongLiteralValue*>
                (expr -> right_expression -> value);

            expr -> value =
                control.int_pool.FindOrInsert(left -> value <=
                                              right -> value ? 1 : 0);
        }
        else if (left_type == control.int_type)
        {
            IntLiteralValue* left = DYNAMIC_CAST<IntLiteralValue*>
                (expr -> left_expression -> value);
            IntLiteralValue* right = DYNAMIC_CAST<IntLiteralValue*>
                (expr -> right_expression -> value);

            expr -> value =
                control.int_pool.FindOrInsert(left -> value <=
                                              right -> value ? 1 : 0);
        }
    }
}


void Semantic::ProcessGREATER_EQUAL(AstBinaryExpression* expr)
{
    ProcessExpression(expr -> left_expression);
    ProcessExpression(expr -> right_expression);

    BinaryNumericPromotion(expr);
    TypeSymbol* left_type = expr -> left_expression -> Type();
    TypeSymbol* right_type = expr -> right_expression -> Type();

    expr -> symbol = (left_type == control.no_type ||
                      right_type == control.no_type)
        ? control.no_type : control.boolean_type;

    if (expr -> left_expression -> IsConstant() &&
        expr -> right_expression -> IsConstant())
    {
        if (left_type == control.double_type)
        {
            DoubleLiteralValue* left = DYNAMIC_CAST<DoubleLiteralValue*>
                (expr -> left_expression -> value);
            DoubleLiteralValue* right = DYNAMIC_CAST<DoubleLiteralValue*>
                (expr -> right_expression -> value);

            expr -> value =
                control.int_pool.FindOrInsert(left -> value >=
                                              right -> value ? 1 : 0);
        }
        else if (left_type == control.float_type)
        {
            FloatLiteralValue* left = DYNAMIC_CAST<FloatLiteralValue*>
                (expr -> left_expression -> value);
            FloatLiteralValue* right = DYNAMIC_CAST<FloatLiteralValue*>
                (expr -> right_expression -> value);

            expr -> value =
                control.int_pool.FindOrInsert(left -> value >=
                                              right -> value ? 1 : 0);
        }
        else if (left_type == control.long_type)
        {
            LongLiteralValue* left = DYNAMIC_CAST<LongLiteralValue*>
                (expr -> left_expression -> value);
            LongLiteralValue* right = DYNAMIC_CAST<LongLiteralValue*>
                (expr -> right_expression -> value);

            expr -> value =
                control.int_pool.FindOrInsert(left -> value >=
                                              right -> value ? 1 : 0);
        }
        else if (left_type == control.int_type)
        {
            IntLiteralValue* left = DYNAMIC_CAST<IntLiteralValue*>
                (expr -> left_expression -> value);
            IntLiteralValue* right = DYNAMIC_CAST<IntLiteralValue*>
                (expr -> right_expression -> value);

            expr -> value =
                control.int_pool.FindOrInsert(left -> value >=
                                              right -> value ? 1 : 0);
        }
    }
}


void Semantic::ProcessAND(AstBinaryExpression* expr)
{
    ProcessExpression(expr -> left_expression);
    ProcessExpression(expr -> right_expression);

    TypeSymbol* left_type = expr -> left_expression -> Type();
    TypeSymbol* right_type = expr -> right_expression -> Type();

    // Java 5: Unbox Boolean to boolean
    if (left_type == control.Boolean())
    {
        expr -> left_expression = ConvertToType(expr -> left_expression, control.boolean_type);
        left_type = control.boolean_type;
    }
    if (right_type == control.Boolean())
    {
        expr -> right_expression = ConvertToType(expr -> right_expression, control.boolean_type);
        right_type = control.boolean_type;
    }

    if (left_type == control.boolean_type ||
        right_type == control.boolean_type)
    {
        if (left_type != control.boolean_type)
        {
            if (left_type != control.no_type)
                ReportSemError(SemanticError::TYPE_NOT_BOOLEAN,
                               expr -> left_expression,
                               left_type -> ContainingPackageName(),
                               left_type -> ExternalName());
            expr -> symbol = control.no_type;
        }
        else if (right_type != control.boolean_type)
        {
            if (right_type != control.no_type)
                ReportSemError(SemanticError::TYPE_NOT_BOOLEAN,
                               expr -> right_expression,
                               right_type -> ContainingPackageName(),
                               right_type -> ExternalName());
            expr -> symbol = control.no_type;
        }
        else expr -> symbol = control.boolean_type;
        if (expr -> left_expression -> IsConstant() &&
            expr -> right_expression -> IsConstant())
        {
            expr -> value = control.int_pool
                .FindOrInsert((IsConstantTrue(expr -> left_expression) &&
                               IsConstantTrue(expr -> right_expression))
                              ? 1 : 0);
        }
    }
    else
    {
        BinaryNumericPromotion(expr);
        TypeSymbol* expr_type = expr -> Type();

        if (! control.IsIntegral(expr_type))
        {
            if (! control.IsIntegral(left_type) &&
                left_type != control.no_type)
            {
                ReportSemError(SemanticError::TYPE_NOT_INTEGRAL,
                               expr -> left_expression,
                               left_type -> ContainingPackageName(),
                               left_type -> ExternalName());
            }
            if (! control.IsIntegral(right_type) &&
                right_type != control.no_type)
            {
                ReportSemError(SemanticError::TYPE_NOT_INTEGRAL,
                               expr -> right_expression,
                               right_type -> ContainingPackageName(),
                               right_type -> ExternalName());
            }
            expr -> symbol = control.no_type;
        }
        if (expr -> left_expression -> IsConstant() &&
            expr -> right_expression -> IsConstant())
        {
            if (expr_type == control.long_type)
            {
                LongLiteralValue* left = DYNAMIC_CAST<LongLiteralValue*>
                    (expr -> left_expression -> value);
                LongLiteralValue* right = DYNAMIC_CAST<LongLiteralValue*>
                    (expr -> right_expression -> value);

                expr -> value = control.long_pool.FindOrInsert(left -> value &
                                                               right -> value);
            }
            else if (expr_type == control.int_type)
            {
                IntLiteralValue* left = DYNAMIC_CAST<IntLiteralValue*>
                    (expr -> left_expression -> value);
                IntLiteralValue* right = DYNAMIC_CAST<IntLiteralValue*>
                    (expr -> right_expression -> value);

                expr -> value = control.int_pool.FindOrInsert(left -> value &
                                                              right -> value);
            }
        }
    }
}


void Semantic::ProcessXOR(AstBinaryExpression* expr)
{
    ProcessExpression(expr -> left_expression);
    ProcessExpression(expr -> right_expression);

    TypeSymbol* left_type = expr -> left_expression -> Type();
    TypeSymbol* right_type = expr -> right_expression -> Type();

    // Java 5: Unbox Boolean to boolean
    if (left_type == control.Boolean())
    {
        expr -> left_expression = ConvertToType(expr -> left_expression, control.boolean_type);
        left_type = control.boolean_type;
    }
    if (right_type == control.Boolean())
    {
        expr -> right_expression = ConvertToType(expr -> right_expression, control.boolean_type);
        right_type = control.boolean_type;
    }

    if (left_type == control.boolean_type ||
        right_type == control.boolean_type)
    {
        if (left_type != control.boolean_type)
        {
            if (left_type != control.no_type)
                ReportSemError(SemanticError::TYPE_NOT_BOOLEAN,
                               expr -> left_expression,
                               left_type -> ContainingPackageName(),
                               left_type -> ExternalName());
            expr -> symbol = control.no_type;
        }
        else if (right_type != control.boolean_type)
        {
            if (right_type != control.no_type)
                ReportSemError(SemanticError::TYPE_NOT_BOOLEAN,
                               expr -> right_expression,
                               right_type -> ContainingPackageName(),
                               right_type -> ExternalName());
            expr -> symbol = control.no_type;
        }
        else expr -> symbol = control.boolean_type;
        if (expr -> left_expression -> IsConstant() &&
            expr -> right_expression -> IsConstant())
        {
            expr -> value = control.int_pool
                .FindOrInsert((IsConstantTrue(expr -> left_expression) !=
                               IsConstantTrue(expr -> right_expression))
                              ? 1 : 0);
        }
    }
    else
    {
        BinaryNumericPromotion(expr);
        TypeSymbol* expr_type = expr -> Type();

        if (! control.IsIntegral(expr_type))
        {
            if (! control.IsIntegral(left_type) &&
                left_type != control.no_type)
            {
                ReportSemError(SemanticError::TYPE_NOT_INTEGRAL,
                               expr -> left_expression,
                               left_type -> ContainingPackageName(),
                               left_type -> ExternalName());
            }
            if (! control.IsIntegral(right_type) &&
                right_type != control.no_type)
            {
                ReportSemError(SemanticError::TYPE_NOT_INTEGRAL,
                               expr -> right_expression,
                               right_type -> ContainingPackageName(),
                               right_type -> ExternalName());
            }
            expr -> symbol = control.no_type;
        }
        if (expr -> left_expression -> IsConstant() &&
            expr -> right_expression -> IsConstant())
        {
            if (expr_type == control.long_type)
            {
                LongLiteralValue* left = DYNAMIC_CAST<LongLiteralValue*>
                    (expr -> left_expression -> value);
                LongLiteralValue* right = DYNAMIC_CAST<LongLiteralValue*>
                    (expr -> right_expression -> value);

                expr -> value = control.long_pool.FindOrInsert(left -> value ^
                                                               right -> value);
            }
            else if (expr_type == control.int_type)
            {
                IntLiteralValue* left = DYNAMIC_CAST<IntLiteralValue*>
                    (expr -> left_expression -> value);
                IntLiteralValue* right = DYNAMIC_CAST<IntLiteralValue*>
                    (expr -> right_expression -> value);

                expr -> value = control.int_pool.FindOrInsert(left -> value ^
                                                              right -> value);
            }
        }
    }
}


void Semantic::ProcessIOR(AstBinaryExpression* expr)
{
    ProcessExpression(expr -> left_expression);
    ProcessExpression(expr -> right_expression);

    TypeSymbol* left_type = expr -> left_expression -> Type();
    TypeSymbol* right_type = expr -> right_expression -> Type();

    // Java 5: Unbox Boolean to boolean
    if (left_type == control.Boolean())
    {
        expr -> left_expression = ConvertToType(expr -> left_expression, control.boolean_type);
        left_type = control.boolean_type;
    }
    if (right_type == control.Boolean())
    {
        expr -> right_expression = ConvertToType(expr -> right_expression, control.boolean_type);
        right_type = control.boolean_type;
    }

    if (left_type == control.boolean_type ||
        right_type == control.boolean_type)
    {
        if (left_type != control.boolean_type)
        {
            if (left_type != control.no_type)
                ReportSemError(SemanticError::TYPE_NOT_BOOLEAN,
                               expr -> left_expression,
                               left_type -> ContainingPackageName(),
                               left_type -> ExternalName());
            expr -> symbol = control.no_type;
        }
        else if (right_type != control.boolean_type)
        {
            if (right_type != control.no_type)
                ReportSemError(SemanticError::TYPE_NOT_BOOLEAN,
                               expr -> right_expression,
                               right_type -> ContainingPackageName(),
                               right_type -> ExternalName());
            expr -> symbol = control.no_type;
        }
        else expr -> symbol = control.boolean_type;
        if (expr -> left_expression -> IsConstant() &&
            expr -> right_expression -> IsConstant())
        {
            expr -> value = control.int_pool
                .FindOrInsert((IsConstantTrue(expr -> left_expression) ||
                               IsConstantTrue(expr -> right_expression))
                              ? 1 : 0);
        }
    }
    else
    {
        BinaryNumericPromotion(expr);
        TypeSymbol* expr_type = expr -> Type();

        if (! control.IsIntegral(expr_type))
        {
            if (! control.IsIntegral(left_type) &&
                left_type != control.no_type)
            {
                ReportSemError(SemanticError::TYPE_NOT_INTEGRAL,
                               expr -> left_expression,
                               left_type -> ContainingPackageName(),
                               left_type -> ExternalName());
            }
            if (! control.IsIntegral(right_type) &&
                right_type != control.no_type)
            {
                ReportSemError(SemanticError::TYPE_NOT_INTEGRAL,
                               expr -> right_expression,
                               right_type -> ContainingPackageName(),
                               right_type -> ExternalName());
            }
            expr -> symbol = control.no_type;
        }
        if (expr -> left_expression -> IsConstant() &&
            expr -> right_expression -> IsConstant())
        {
            if (expr_type == control.long_type)
            {
                LongLiteralValue* left = DYNAMIC_CAST<LongLiteralValue*>
                    (expr -> left_expression -> value);
                LongLiteralValue* right = DYNAMIC_CAST<LongLiteralValue*>
                    (expr -> right_expression -> value);

                expr -> value = control.long_pool.FindOrInsert(left -> value |
                                                               right -> value);
            }
            else if (expr_type == control.int_type)
            {
                IntLiteralValue* left = DYNAMIC_CAST<IntLiteralValue*>
                    (expr -> left_expression -> value);
                IntLiteralValue* right = DYNAMIC_CAST<IntLiteralValue*>
                    (expr -> right_expression -> value);

                expr -> value = control.int_pool.FindOrInsert(left -> value |
                                                              right -> value);
            }
        }
    }
}


void Semantic::ProcessAND_AND(AstBinaryExpression* expr)
{
    ProcessExpression(expr -> left_expression);
    ProcessExpression(expr -> right_expression);

    TypeSymbol* left_type = expr -> left_expression -> Type();
    TypeSymbol* right_type = expr -> right_expression -> Type();

    // Java 5: Unbox Boolean to boolean
    if (left_type == control.Boolean())
    {
        expr -> left_expression = ConvertToType(expr -> left_expression, control.boolean_type);
        left_type = control.boolean_type;
    }
    if (right_type == control.Boolean())
    {
        expr -> right_expression = ConvertToType(expr -> right_expression, control.boolean_type);
        right_type = control.boolean_type;
    }

    if (left_type != control.boolean_type)
    {
        if (left_type != control.no_type)
            ReportSemError(SemanticError::TYPE_NOT_BOOLEAN,
                           expr -> left_expression,
                           left_type -> ContainingPackageName(),
                           left_type -> ExternalName());
        expr -> symbol = control.no_type;
    }
    if (right_type != control.boolean_type)
    {
        if (right_type != control.no_type)
            ReportSemError(SemanticError::TYPE_NOT_BOOLEAN,
                           expr -> right_expression,
                           right_type -> ContainingPackageName(),
                           right_type -> ExternalName());
        expr -> symbol = control.no_type;
    }
    if (expr -> left_expression -> IsConstant() &&
        expr -> right_expression -> IsConstant())
    {
        //
        // Even when evaluating false && x, x must be constant for && to
        // be a constant expression according to JLS2 15.28.
        //
        expr -> value = control.int_pool.
            FindOrInsert((IsConstantTrue(expr -> left_expression) &&
                          IsConstantTrue(expr -> right_expression))
                         ? 1 : 0);
    }
    if (expr -> symbol != control.no_type)
        expr -> symbol = control.boolean_type;
}


void Semantic::ProcessOR_OR(AstBinaryExpression* expr)
{
    ProcessExpression(expr -> left_expression);
    ProcessExpression(expr -> right_expression);

    TypeSymbol* left_type = expr -> left_expression -> Type();
    TypeSymbol* right_type = expr -> right_expression -> Type();

    // Java 5: Unbox Boolean to boolean
    if (left_type == control.Boolean())
    {
        expr -> left_expression = ConvertToType(expr -> left_expression, control.boolean_type);
        left_type = control.boolean_type;
    }
    if (right_type == control.Boolean())
    {
        expr -> right_expression = ConvertToType(expr -> right_expression, control.boolean_type);
        right_type = control.boolean_type;
    }

    if (left_type != control.boolean_type)
    {
        if (left_type != control.no_type)
            ReportSemError(SemanticError::TYPE_NOT_BOOLEAN,
                           expr -> left_expression,
                           left_type -> ContainingPackageName(),
                           left_type -> ExternalName());
        expr -> symbol = control.no_type;
    }
    if (right_type != control.boolean_type)
    {
        if (right_type != control.no_type)
            ReportSemError(SemanticError::TYPE_NOT_BOOLEAN,
                           expr -> right_expression,
                           right_type -> ContainingPackageName(),
                           right_type -> ExternalName());
        expr -> symbol = control.no_type;
    }
    if (expr -> left_expression -> IsConstant() &&
        expr -> right_expression -> IsConstant())
    {
        //
        // Even when evaluating true || x, x must be constant for || to
        // be a constant expression according to JLS2 15.28.
        //
        expr -> value = control.int_pool.
            FindOrInsert((IsConstantTrue(expr -> left_expression) ||
                          IsConstantTrue(expr -> right_expression))
                         ? 1 : 0);
    }
    if (expr -> symbol != control.no_type)
        expr -> symbol = control.boolean_type;
}


void Semantic::ProcessEQUAL_EQUAL(AstBinaryExpression* expr)
{
    ProcessExpressionOrStringConstant(expr -> left_expression);
    ProcessExpressionOrStringConstant(expr -> right_expression);

    TypeSymbol* left_type = expr -> left_expression -> Type();
    TypeSymbol* right_type = expr -> right_expression -> Type();

    if (left_type == control.void_type || right_type == control.void_type)
    {
        if (left_type == control.void_type)
            ReportSemError(SemanticError::TYPE_IS_VOID,
                           expr -> left_expression,
                           left_type -> Name());
        if (right_type == control.void_type)
            ReportSemError(SemanticError::TYPE_IS_VOID,
                           expr -> right_expression,
                           right_type -> Name());
        expr -> symbol = control.no_type;
    }
    else if (left_type -> Primitive() && right_type -> Primitive())
    {
        if (left_type == control.boolean_type ||
            right_type == control.boolean_type)
        {
            if (left_type != right_type)
            {
                ReportSemError(SemanticError::INCOMPATIBLE_TYPE_FOR_BINARY_EXPRESSION,
                               expr, left_type -> ContainingPackageName(),
                               left_type -> ExternalName(),
                               right_type -> ContainingPackageName(),
                               right_type -> ExternalName());
                expr -> symbol = control.no_type;
            }
        }
        else BinaryNumericPromotion(expr);
        if (expr -> symbol != control.no_type)
            expr -> symbol = control.boolean_type;
    }
    else if (CanCastConvert(left_type, right_type,
                            expr -> binary_operator_token) ||
             (left_type == control.null_type &&
              (right_type == control.null_type ||
               right_type -> IsSubclass(control.Object()))))
    {
        expr -> symbol = control.boolean_type;
    }
    else
    {
        ReportSemError(SemanticError::INCOMPATIBLE_TYPE_FOR_BINARY_EXPRESSION,
                       expr, left_type -> ContainingPackageName(),
                       left_type -> ExternalName(),
                       right_type -> ContainingPackageName(),
                       right_type -> ExternalName());
        expr -> symbol = control.no_type;
    }

    if (expr -> left_expression -> IsConstant() &&
        expr -> right_expression -> IsConstant())
    {
        LiteralValue* left = expr -> left_expression -> value;
        LiteralValue* right = expr -> right_expression -> value;

        //
        // Check double and float separately from long, int, and String; since
        // 0.0 and NaNs cause weird behavior.
        //
        if (expr -> left_expression -> Type() == control.double_type)
        {
            DoubleLiteralValue* left = DYNAMIC_CAST<DoubleLiteralValue*>
                (expr -> left_expression -> value);
            DoubleLiteralValue* right = DYNAMIC_CAST<DoubleLiteralValue*>
                (expr -> right_expression -> value);
            expr -> value =
                control.int_pool.FindOrInsert(left -> value ==
                                              right -> value ? 1 : 0);
        }
        else if (expr -> left_expression -> Type() == control.float_type)
        {
            FloatLiteralValue* left = DYNAMIC_CAST<FloatLiteralValue*>
                (expr -> left_expression -> value);
            FloatLiteralValue* right = DYNAMIC_CAST<FloatLiteralValue*>
                (expr -> right_expression -> value);
            expr -> value =
                control.int_pool.FindOrInsert(left -> value ==
                                              right -> value ? 1 : 0);
        }
        else expr -> value =
                 control.int_pool.FindOrInsert(left == right ? 1 : 0);
    }
}


void Semantic::ProcessNOT_EQUAL(AstBinaryExpression* expr)
{
    ProcessExpressionOrStringConstant(expr -> left_expression);
    ProcessExpressionOrStringConstant(expr -> right_expression);

    TypeSymbol* left_type = expr -> left_expression -> Type();
    TypeSymbol* right_type = expr -> right_expression -> Type();

    if (left_type == control.void_type || right_type == control.void_type)
    {
        if (left_type == control.void_type)
            ReportSemError(SemanticError::TYPE_IS_VOID,
                           expr -> left_expression,
                           left_type -> Name());
        if (right_type == control.void_type)
            ReportSemError(SemanticError::TYPE_IS_VOID,
                           expr -> right_expression,
                           right_type -> Name());
        expr -> symbol = control.no_type;
    }
    else if (left_type -> Primitive() && right_type -> Primitive())
    {
        if (left_type == control.boolean_type ||
            right_type == control.boolean_type)
        {
            if (left_type != right_type)
            {
                ReportSemError(SemanticError::INCOMPATIBLE_TYPE_FOR_BINARY_EXPRESSION,
                               expr, left_type -> ContainingPackageName(),
                               left_type -> ExternalName(),
                               right_type -> ContainingPackageName(),
                               right_type -> ExternalName());
                expr -> symbol = control.no_type;
            }
        }
        else BinaryNumericPromotion(expr);
        if (expr -> symbol != control.no_type)
            expr -> symbol = control.boolean_type;
    }
    else if (CanCastConvert(left_type, right_type,
                            expr -> binary_operator_token) ||
             (left_type == control.null_type &&
              (right_type == control.null_type ||
               right_type -> IsSubclass(control.Object()))))
    {
        expr -> symbol = control.boolean_type;
    }
    else
    {
        ReportSemError(SemanticError::INCOMPATIBLE_TYPE_FOR_BINARY_EXPRESSION,
                       expr, left_type -> ContainingPackageName(),
                       left_type -> ExternalName(),
                       right_type -> ContainingPackageName(),
                       right_type -> ExternalName());
        expr -> symbol = control.no_type;
    }

    if (expr -> left_expression -> IsConstant() &&
        expr -> right_expression -> IsConstant())
    {
        LiteralValue* left = expr -> left_expression -> value;
        LiteralValue* right = expr -> right_expression -> value;

        //
        // Check double and float separately from long, int, and String; since
        // 0.0 and NaNs cause weird behavior.
        //
        if (expr -> left_expression -> Type() == control.double_type)
        {
            DoubleLiteralValue* left = DYNAMIC_CAST<DoubleLiteralValue*>
                (expr -> left_expression -> value);
            DoubleLiteralValue* right = DYNAMIC_CAST<DoubleLiteralValue*>
                (expr -> right_expression -> value);

            expr -> value =
                control.int_pool.FindOrInsert(left -> value !=
                                              right -> value ? 1 : 0);
        }
        else if (expr -> left_expression -> Type() == control.float_type)
        {
            FloatLiteralValue* left = DYNAMIC_CAST<FloatLiteralValue*>
                (expr -> left_expression -> value);
            FloatLiteralValue* right = DYNAMIC_CAST<FloatLiteralValue*>
                (expr -> right_expression -> value);

            expr -> value =
                control.int_pool.FindOrInsert(left -> value !=
                                              right -> value ? 1 : 0);
        }
        else expr -> value =
                 control.int_pool.FindOrInsert(left != right ? 1 : 0);
    }
}


void Semantic::ProcessSTAR(AstBinaryExpression* expr)
{
    ProcessExpression(expr -> left_expression);
    ProcessExpression(expr -> right_expression);
    BinaryNumericPromotion(expr);

    if (expr -> left_expression -> IsConstant() &&
        expr -> right_expression -> IsConstant())
    {
        if (expr -> Type() == control.double_type)
        {
            DoubleLiteralValue* left = DYNAMIC_CAST<DoubleLiteralValue*>
                (expr -> left_expression -> value);
            DoubleLiteralValue* right = DYNAMIC_CAST<DoubleLiteralValue*>
                (expr -> right_expression -> value);

            expr -> value =
                control.double_pool.FindOrInsert(left -> value *
                                                 right -> value);
        }
        else if (expr -> Type() == control.float_type)
        {
            FloatLiteralValue* left = DYNAMIC_CAST<FloatLiteralValue*>
                (expr -> left_expression -> value);
            FloatLiteralValue* right = DYNAMIC_CAST<FloatLiteralValue*>
                (expr -> right_expression -> value);

            expr -> value =
                control.float_pool.FindOrInsert(left -> value *
                                                right -> value);
        }
        else if (expr -> Type() == control.long_type)
        {
            LongLiteralValue* left = DYNAMIC_CAST<LongLiteralValue*>
                (expr -> left_expression -> value);
            LongLiteralValue* right = DYNAMIC_CAST<LongLiteralValue*>
                (expr -> right_expression -> value);
            CheckIntegerMultiplication(this, expr,
                                       left -> value, right -> value);
            expr -> value =
                control.long_pool.FindOrInsert(left -> value *
                                               right -> value);
        }
        else if (expr -> Type() == control.int_type)
        {
            IntLiteralValue* left = DYNAMIC_CAST<IntLiteralValue*>
                (expr -> left_expression -> value);
            IntLiteralValue* right = DYNAMIC_CAST<IntLiteralValue*>
                (expr -> right_expression -> value);
            CheckIntegerMultiplication(this, expr,
                                       left -> value, right -> value);
            expr -> value =
                control.int_pool.FindOrInsert(left -> value *
                                              right -> value);
        }
    }
}


void Semantic::ProcessMINUS(AstBinaryExpression* expr)
{
    ProcessExpression(expr -> left_expression);
    ProcessExpression(expr -> right_expression);
    BinaryNumericPromotion(expr);

    if (expr -> left_expression -> IsConstant() &&
        expr -> right_expression -> IsConstant())
    {
        if (expr -> Type() == control.double_type)
        {
            DoubleLiteralValue* left = DYNAMIC_CAST<DoubleLiteralValue*>
                (expr -> left_expression -> value);
            DoubleLiteralValue* right = DYNAMIC_CAST<DoubleLiteralValue*>
                (expr -> right_expression -> value);

            expr -> value =
                control.double_pool.FindOrInsert(left -> value -
                                                 right -> value);
        }
        else if (expr -> Type() == control.float_type)
        {
            FloatLiteralValue* left = DYNAMIC_CAST<FloatLiteralValue*>
                (expr -> left_expression -> value);
            FloatLiteralValue* right = DYNAMIC_CAST<FloatLiteralValue*>
                (expr -> right_expression -> value);

            expr -> value =
                control.float_pool.FindOrInsert(left -> value -
                                                right -> value);
        }
        else if (expr -> Type() == control.long_type)
        {
            LongLiteralValue* left = DYNAMIC_CAST<LongLiteralValue*>
                (expr -> left_expression -> value);
            LongLiteralValue* right = DYNAMIC_CAST<LongLiteralValue*>
                (expr -> right_expression -> value);
            CheckIntegerSubtraction(this, expr, left -> value, right -> value);
            expr -> value =
                control.long_pool.FindOrInsert(left -> value -
                                               right -> value);
        }
        else if (expr -> Type() == control.int_type)
        {
            IntLiteralValue* left = DYNAMIC_CAST<IntLiteralValue*>
                (expr -> left_expression -> value);
            IntLiteralValue* right = DYNAMIC_CAST<IntLiteralValue*>
                (expr -> right_expression -> value);
            CheckIntegerSubtraction(this, expr, left -> value, right -> value);
            expr -> value =
                control.int_pool.FindOrInsert(left -> value - right -> value);
        }
    }
}


void Semantic::ProcessSLASH(AstBinaryExpression* expr)
{
    ProcessExpression(expr -> left_expression);
    ProcessExpression(expr -> right_expression);
    BinaryNumericPromotion(expr);

    AstExpression* left_expression = expr -> left_expression;
    AstExpression* right_expression = expr -> right_expression;
    if (right_expression -> IsConstant())
    {
        //
        // If the type of the expression is int or long and the right-hand
        // side is 0 then issue an error message. Otherwise, if both
        // subexpressions are constant, calculate result.
        //
        if ((expr -> Type() == control.int_type &&
             DYNAMIC_CAST<IntLiteralValue*> (right_expression -> value) -> value == 0) ||
            (expr -> Type() == control.long_type &&
             DYNAMIC_CAST<LongLiteralValue*> (right_expression -> value) -> value == 0))
        {
            //
            // This will guarantee a runtime exception, but the
            // clarifications to JLS2 insist it is legal code.
            //
            ReportSemError(SemanticError::ZERO_DIVIDE_CAUTION, expr);
        }
        else if (left_expression -> IsConstant())
        {
            if (expr -> Type() == control.double_type)
            {
                DoubleLiteralValue* left = DYNAMIC_CAST<DoubleLiteralValue*>
                    (left_expression -> value);
                DoubleLiteralValue* right = DYNAMIC_CAST<DoubleLiteralValue*>
                    (right_expression -> value);

                expr -> value =
                    control.double_pool.FindOrInsert(left -> value /
                                                     right -> value);
            }
            else if (expr -> Type() == control.float_type)
            {
                FloatLiteralValue* left = DYNAMIC_CAST<FloatLiteralValue*>
                    (left_expression -> value);
                FloatLiteralValue* right = DYNAMIC_CAST<FloatLiteralValue*>
                    (right_expression -> value);

                expr -> value =
                    control.float_pool.FindOrInsert(left -> value /
                                                    right -> value);
            }
            else if (expr -> Type() == control.long_type)
            {
                LongLiteralValue* left = DYNAMIC_CAST<LongLiteralValue*>
                    (left_expression -> value);
                LongLiteralValue* right = DYNAMIC_CAST<LongLiteralValue*>
                    (right_expression -> value);
                CheckIntegerDivision(this, expr, left -> value,
                                     right -> value);
                expr -> value =
                    control.long_pool.FindOrInsert(left -> value /
                                                   right -> value);
            }
            else if (expr -> Type() == control.int_type)
            {
                IntLiteralValue* left = DYNAMIC_CAST<IntLiteralValue*>
                    (left_expression -> value);
                IntLiteralValue* right = DYNAMIC_CAST<IntLiteralValue*>
                    (right_expression -> value);
                CheckIntegerDivision(this, expr, left -> value,
                                     right -> value);
                //
                // There is a bug in the intel hardware where if one tries
                // to compute ((2**32-1) / -1), he gets a ZeroDivide
                // exception. Thus, instead of using the straightforward
                // code below, we use the short-circuited one that follows:
                //
                //  expr -> value = control.int_pool
                //      .FindOrInsert(left -> value / right -> value);
                //
                expr -> value = control.int_pool
                    .FindOrInsert(right -> value == -1
                                  ? -(left -> value)
                                  : left -> value / right -> value);
            }
        }
    }
}


void Semantic::ProcessMOD(AstBinaryExpression* expr)
{
    ProcessExpression(expr -> left_expression);
    ProcessExpression(expr -> right_expression);
    BinaryNumericPromotion(expr);

    AstExpression* left_expression = expr -> left_expression;
    AstExpression* right_expression = expr -> right_expression;
    if (right_expression -> IsConstant())
    {
        //
        // If the type of the expression is int or long and the right-hand
        // side is 0 then issue an error message. Otherwise, if both
        // subexpressions are constant, calculate result.
        //
        if ((expr -> Type() == control.int_type &&
             DYNAMIC_CAST<IntLiteralValue*> (right_expression -> value) -> value == 0) ||
            (expr -> Type() == control.long_type &&
             DYNAMIC_CAST<LongLiteralValue*> (right_expression -> value) -> value == 0))
        {
            //
            // This will guarantee a runtime exception, but the
            // clarifications to JLS2 insist it is legal code.
            //
            ReportSemError(SemanticError::ZERO_DIVIDE_CAUTION, expr);
        }
        else if (left_expression -> IsConstant())
        {
            if (expr -> Type() == control.double_type)
            {
                DoubleLiteralValue* left = DYNAMIC_CAST<DoubleLiteralValue*>
                    (left_expression -> value);
                DoubleLiteralValue* right = DYNAMIC_CAST<DoubleLiteralValue*>
                    (right_expression -> value);

                expr -> value =
                    control.double_pool.FindOrInsert(left -> value %
                                                     right -> value);
            }
            else if (expr -> Type() == control.float_type)
            {
                FloatLiteralValue* left = DYNAMIC_CAST<FloatLiteralValue*>
                    (left_expression -> value);
                FloatLiteralValue* right = DYNAMIC_CAST<FloatLiteralValue*>
                    (right_expression -> value);

                expr -> value =
                    control.float_pool.FindOrInsert(left -> value %
                                                    right -> value);
            }
            else if (expr -> Type() == control.long_type)
            {
                LongLiteralValue* left = DYNAMIC_CAST<LongLiteralValue*>
                    (left_expression -> value);
                LongLiteralValue* right = DYNAMIC_CAST<LongLiteralValue*>
                    (right_expression -> value);

                expr -> value =
                    control.long_pool.FindOrInsert(left -> value %
                                                   right -> value);
            }
            else if (expr -> Type() == control.int_type)
            {
                IntLiteralValue* left = DYNAMIC_CAST<IntLiteralValue*>
                    (left_expression -> value);
                IntLiteralValue* right = DYNAMIC_CAST<IntLiteralValue*>
                    (right_expression -> value);

                //
                // There is a bug in the intel hardware where if one tries
                // to compute ((2**32-1) / -1), he gets a ZeroDivide
                // exception. Thus, instead of using the straightforward
                // code below, we use the short-circuited one that follows:
                //
                // expr -> value = control.int_pool
                //     .FindOrInsert(left -> value % right -> value);
                //
                expr -> value = control.int_pool
                    .FindOrInsert((left -> value  == (signed) 0x80000000 &&
                                   right -> value == (signed) 0xffffffff)
                                  ? 0 : left -> value % right -> value);
            }
        }
    }
}


void Semantic::ProcessBinaryExpression(Ast* expr)
{
    AstBinaryExpression* binary_expression = (AstBinaryExpression*) expr;
    (this ->* ProcessBinaryExpr[binary_expression -> Tag()])
        (binary_expression);
}


void Semantic::ProcessInstanceofExpression(Ast* expr)
{
    AstInstanceofExpression* instanceof = (AstInstanceofExpression*) expr;
    ProcessExpressionOrStringConstant(instanceof -> expression);
    ProcessType(instanceof -> type);

    TypeSymbol* left_type = instanceof -> expression -> Type();
    TypeSymbol* right_type = instanceof -> type -> symbol;

    if (left_type -> Primitive())
    {
        ReportSemError(SemanticError::TYPE_NOT_REFERENCE,
                       instanceof -> expression,
                       left_type -> Name());
        instanceof -> symbol = control.no_type;
    }
    // Check for illegal instanceof with parameterized types
    // Due to type erasure, instanceof cannot check type arguments at runtime
    // Example: obj instanceof List<String> is illegal
    // But unbounded wildcards like List<?> are allowed (JLS 15.20.2)
    else if (control.option.source >= JopaOption::SDK1_5)
    {
        // Check for illegal instanceof with parameterized types that use
        // concrete type arguments. Unbounded wildcards like List<?> are allowed.
        AstTypeName* type_name = instanceof -> type -> TypeNameCast();
        bool has_illegal_type_arg = false;
        if (type_name && type_name -> type_arguments_opt)
        {
            AstTypeArguments* type_args = type_name -> type_arguments_opt;
            for (unsigned i = 0; i < type_args -> NumTypeArguments(); i++)
            {
                AstWildcard* wildcard = type_args -> TypeArgument(i) -> WildcardCast();
                // Not a wildcard, or wildcard with bounds (extends/super)
                if (!wildcard || wildcard -> bounds_opt)
                {
                    has_illegal_type_arg = true;
                    break;
                }
            }
        }
        if (has_illegal_type_arg)
        {
            // instanceof with parameterized type is illegal due to type erasure
            // Use instanceof with raw type and cast to parameterized type instead
            ReportSemError(SemanticError::INVALID_INSTANCEOF_CONVERSION,
                           instanceof -> type,
                           left_type -> ContainingPackageName(),
                           left_type -> ExternalName(),
                           right_type -> ContainingPackageName(),
                           right_type -> ExternalName());
            instanceof -> symbol = control.no_type;
        }
        // can left_type (source) be cast into right_type
        else if (! CanCastConvert(right_type, left_type,
                                  instanceof -> instanceof_token))
        {
            ReportSemError(SemanticError::INVALID_INSTANCEOF_CONVERSION,
                           expr, left_type -> ContainingPackageName(),
                           left_type -> ExternalName(),
                           right_type -> ContainingPackageName(),
                           right_type -> ExternalName());
            instanceof -> symbol = control.no_type;
        }
        else instanceof -> symbol = control.boolean_type;
    }
    // Pre-1.5 behavior
    else if (! CanCastConvert(right_type, left_type,
                              instanceof -> instanceof_token))
    {
        ReportSemError(SemanticError::INVALID_INSTANCEOF_CONVERSION,
                       expr, left_type -> ContainingPackageName(),
                       left_type -> ExternalName(),
                       right_type -> ContainingPackageName(),
                       right_type -> ExternalName());
        instanceof -> symbol = control.no_type;
    }
    else instanceof -> symbol = control.boolean_type;
}


void Semantic::ProcessConditionalExpression(Ast* expr)
{
    AstConditionalExpression* conditional_expression =
        (AstConditionalExpression*) expr;

    ProcessExpression(conditional_expression -> test_expression);
    //
    // TODO: Should we delay calculating results of true/false expressions
    // until CheckStringConstant in lookup.cpp to put fewer intermediate
    // strings in the storage pools?
    //
    ProcessExpressionOrStringConstant(conditional_expression ->
                                      true_expression);
    ProcessExpressionOrStringConstant(conditional_expression ->
                                      false_expression);

    TypeSymbol* test_type =
        conditional_expression -> test_expression -> Type();
    TypeSymbol* true_type =
        conditional_expression -> true_expression -> Type();
    TypeSymbol* false_type =
        conditional_expression -> false_expression -> Type();

    // Java 5: Unbox Boolean to boolean for condition
    if (test_type == control.Boolean())
    {
        conditional_expression -> test_expression =
            ConvertToType(conditional_expression -> test_expression, control.boolean_type);
        test_type = control.boolean_type;
    }

    if (test_type != control.boolean_type)
    {
        if (test_type != control.no_type)
            ReportSemError(SemanticError::TYPE_NOT_BOOLEAN,
                           conditional_expression -> test_expression,
                           test_type -> ContainingPackageName(),
                           test_type -> ExternalName());
        conditional_expression -> symbol = control.no_type;
    }
    if (true_type == control.void_type)
    {
        ReportSemError(SemanticError::TYPE_IS_VOID,
                       conditional_expression -> true_expression,
                       true_type -> Name());
        true_type = control.no_type;
    }
    if (false_type == control.void_type)
    {
        ReportSemError(SemanticError::TYPE_IS_VOID,
                       conditional_expression -> false_expression,
                       false_type -> Name());
        false_type = control.no_type;
    }
    if (true_type == control.no_type || false_type == control.no_type)
        conditional_expression -> symbol = control.no_type;
    else if (true_type -> Primitive())
    {
        if (! false_type -> Primitive() ||
            (true_type != false_type &&
             (true_type == control.boolean_type ||
              false_type == control.boolean_type)))
        {
            // Java 5+: Handle primitive vs reference via boxing
            if (control.option.source >= JopaOption::SDK1_5 &&
                ! false_type -> Primitive())
            {
                // Check if false_type is the boxed form of true_type
                TypeSymbol* unboxed_false = false_type -> UnboxedType(control);
                if (unboxed_false && unboxed_false == true_type)
                {
                    // Unbox false to match primitive true
                    conditional_expression -> false_expression =
                        ConvertToType(conditional_expression -> false_expression,
                                      true_type);
                    conditional_expression -> symbol = true_type;
                }
                else
                {
                    // Box the primitive and use Object as result type
                    TypeSymbol* boxed_true = true_type -> BoxedType(control);
                    if (boxed_true && boxed_true != true_type)
                    {
                        conditional_expression -> true_expression =
                            ConvertToType(conditional_expression -> true_expression,
                                          boxed_true);
                        // Result is Object (lub of boxed primitive and reference type)
                        conditional_expression -> symbol = control.Object();
                    }
                    else
                    {
                        ReportSemError(SemanticError::INCOMPATIBLE_TYPE_FOR_CONDITIONAL_EXPRESSION,
                                       conditional_expression -> true_expression -> LeftToken(),
                                       conditional_expression -> false_expression -> RightToken(),
                                       true_type -> ContainingPackageName(),
                                       true_type -> ExternalName(),
                                       false_type -> ContainingPackageName(),
                                       false_type -> ExternalName());
                        conditional_expression -> symbol = control.no_type;
                    }
                }
            }
            else
            {
                ReportSemError(SemanticError::INCOMPATIBLE_TYPE_FOR_CONDITIONAL_EXPRESSION,
                               conditional_expression -> true_expression -> LeftToken(),
                               conditional_expression -> false_expression -> RightToken(),
                               true_type -> ContainingPackageName(),
                               true_type -> ExternalName(),
                               false_type -> ContainingPackageName(),
                               false_type -> ExternalName());
                conditional_expression -> symbol = control.no_type;
            }
        }
        else // must be a primitive type
        {
            if (true_type == false_type)
            {
                if (conditional_expression -> symbol != control.no_type)
                    conditional_expression -> symbol = true_type;
            }
            else // must be mixed numeric types
            {
                if (true_type == control.byte_type &&
                    false_type == control.short_type)
                {
                    conditional_expression -> true_expression =
                        ConvertToType(conditional_expression -> true_expression,
                                      control.short_type);
                    conditional_expression -> symbol = control.short_type;
                }
                else if (true_type == control.short_type &&
                         false_type == control.byte_type)
                {
                    conditional_expression -> false_expression =
                        ConvertToType(conditional_expression -> false_expression,
                                      control.short_type);
                    conditional_expression -> symbol = control.short_type;
                }
                else if (true_type == control.int_type &&
                         control.IsSimpleIntegerValueType(true_type) &&
                         IsIntValueRepresentableInType(conditional_expression -> true_expression,
                                                       false_type))
                {
                    conditional_expression -> true_expression =
                        ConvertToType(conditional_expression -> true_expression,
                                      false_type);
                    conditional_expression -> symbol = false_type;
                }
                else if (false_type == control.int_type &&
                         control.IsSimpleIntegerValueType(false_type) &&
                         IsIntValueRepresentableInType(conditional_expression -> false_expression,
                                                       true_type))
                {
                    conditional_expression -> false_expression =
                        ConvertToType(conditional_expression -> false_expression,
                                      true_type);
                    conditional_expression -> symbol = true_type;
                }
                else BinaryNumericPromotion(conditional_expression);
            }

            //
            // Even when evaluating 'true ? constant : x' or
            // 'false ? x : constant', x must be constant for ?: to be a
            // constant expression according to JLS2 15.28.
            //
            if (conditional_expression -> true_expression -> IsConstant() &&
                conditional_expression -> false_expression -> IsConstant())
            {
                if (IsConstantTrue(conditional_expression -> test_expression))
                    conditional_expression -> value =
                        conditional_expression -> true_expression -> value;
                else if (IsConstantFalse(conditional_expression -> test_expression))
                    conditional_expression -> value =
                        conditional_expression -> false_expression -> value;
            }
        }
    }
    else // true_type is reference
    {
        // Java 5+: Handle reference vs primitive via boxing
        if (control.option.source >= JopaOption::SDK1_5 &&
            false_type -> Primitive())
        {
            // Check if true_type is the boxed form of false_type
            TypeSymbol* unboxed_true = true_type -> UnboxedType(control);
            if (unboxed_true && unboxed_true == false_type)
            {
                // Unbox true to match primitive false
                conditional_expression -> true_expression =
                    ConvertToType(conditional_expression -> true_expression,
                                  false_type);
                conditional_expression -> symbol = false_type;
            }
            else
            {
                // Box the primitive and use Object as result type
                TypeSymbol* boxed_false = false_type -> BoxedType(control);
                if (boxed_false && boxed_false != false_type)
                {
                    conditional_expression -> false_expression =
                        ConvertToType(conditional_expression -> false_expression,
                                      boxed_false);
                    // Result is Object (lub of reference type and boxed primitive)
                    conditional_expression -> symbol = control.Object();
                }
                else
                {
                    ReportSemError(SemanticError::INCOMPATIBLE_TYPE_FOR_CONDITIONAL_EXPRESSION,
                                   conditional_expression -> true_expression -> LeftToken(),
                                   conditional_expression -> false_expression -> RightToken(),
                                   true_type -> ContainingPackageName(),
                                   true_type -> ExternalName(),
                                   false_type -> ContainingPackageName(),
                                   false_type -> ExternalName());
                    conditional_expression -> symbol = control.no_type;
                }
            }
        }
        else if (CanAssignmentConvert(false_type,
                                 conditional_expression -> true_expression))
        {
            conditional_expression -> true_expression =
                ConvertToType(conditional_expression -> true_expression,
                              false_type);
            conditional_expression -> symbol = false_type;
        }
        else if (CanAssignmentConvert(true_type,
                                      conditional_expression -> false_expression))
        {
            conditional_expression -> false_expression =
                ConvertToType(conditional_expression -> false_expression,
                              true_type);
            conditional_expression -> symbol = true_type;
        }
        else
        {
            // JLS 15.25.3: If neither operand type is convertible to the other,
            // find the least upper bound (lub) of the two types.
            // For classes, this is the most specific common superclass.
            TypeSymbol* lub = NULL;

            // Try to find common superclass for class types
            if (! true_type -> ACC_INTERFACE() && ! false_type -> ACC_INTERFACE())
            {
                // Walk up true_type's inheritance chain and find first class
                // that false_type is a subclass of
                for (TypeSymbol* ancestor = true_type; ancestor; ancestor = ancestor -> super)
                {
                    if (false_type -> IsSubclass(ancestor))
                    {
                        lub = ancestor;
                        break;
                    }
                }
            }

            if (lub)
            {
                // Found a common superclass - use it as the result type
                conditional_expression -> true_expression =
                    ConvertToType(conditional_expression -> true_expression, lub);
                conditional_expression -> false_expression =
                    ConvertToType(conditional_expression -> false_expression, lub);
                conditional_expression -> symbol = lub;
            }
            else
            {
                ReportSemError(SemanticError::INCOMPATIBLE_TYPE_FOR_CONDITIONAL_EXPRESSION,
                               conditional_expression -> true_expression -> LeftToken(),
                               conditional_expression -> false_expression -> RightToken(),
                               true_type -> ContainingPackageName(),
                               true_type -> ExternalName(),
                               false_type -> ContainingPackageName(),
                               false_type -> ExternalName());
                conditional_expression -> symbol = control.no_type;
            }
        }

        //
        // If all the subexpressions are constants, compute the results and
        // set the value of the expression accordingly.
        //
        // Since null should not be a compile-time constant, the assert
        // should not need to check for null type.
        //
        if (conditional_expression -> true_expression -> IsConstant() &&
            conditional_expression -> false_expression -> IsConstant())
        {
            assert(conditional_expression -> symbol == control.String() ||
                   conditional_expression -> symbol == control.no_type);

            if (IsConstantTrue(conditional_expression -> test_expression))
                conditional_expression -> value =
                    conditional_expression -> true_expression -> value;
            else if (IsConstantFalse(conditional_expression -> test_expression))
                conditional_expression -> value =
                    conditional_expression -> false_expression -> value;
        }
    }
}


void Semantic::ProcessAssignmentExpression(Ast* expr)
{
    AstAssignmentExpression* assignment_expression =
        (AstAssignmentExpression*) expr;
    ProcessExpressionOrStringConstant(assignment_expression -> expression);

    AstExpression* left_hand_side = assignment_expression -> left_hand_side;
    //
    // JLS2 added ability for parenthesized variable to remain a variable.
    // Therefore, the grammar was changed to accept all expressions, to avoid
    // ambiguities, and we must filter out invalid left-hand sides.
    //
    if (left_hand_side -> ParenthesizedExpressionCast())
    {
        ReportSemError(SemanticError::UNNECESSARY_PARENTHESIS, left_hand_side);
        while (left_hand_side -> ParenthesizedExpressionCast())
            left_hand_side = ((AstParenthesizedExpression*) left_hand_side) ->
                expression;
    }

    //
    // JLS2 8.3.2.3 permits simple assignment to a variable that has not
    // yet been declared in an initializer.  If the left_hand_side is a
    // variable, we use ProcessingSimpleAssignment() to inform
    // CheckSimpleName() to treat it specially.
    //
    if ((assignment_expression -> Tag() ==
         AstAssignmentExpression::SIMPLE_EQUAL) &&
        left_hand_side -> NameCast() &&
        ! left_hand_side -> NameCast() -> base_opt)
    {
        ProcessingSimpleAssignment() = true;
    }

    ProcessExpression(left_hand_side);
    ProcessingSimpleAssignment() = false;

    if (! left_hand_side -> IsLeftHandSide())
    {
        ReportSemError(SemanticError::NOT_A_VARIABLE, left_hand_side);
        left_hand_side -> symbol = control.no_type;
        assignment_expression -> symbol = control.no_type;
    }

    TypeSymbol* left_type = left_hand_side -> Type();
    TypeSymbol* right_type = assignment_expression -> expression -> Type();

    if (left_type == control.no_type ||
        right_type == control.no_type || right_type == control.void_type)
    {
        if (right_type == control.void_type)
            ReportSemError(SemanticError::TYPE_IS_VOID,
                           assignment_expression -> expression,
                           right_type -> Name());
        assignment_expression -> symbol = control.no_type;
        return;
    }
    assignment_expression -> symbol = left_type;

    if (! left_hand_side -> ArrayAccessCast()) // the left-hand-side is a name
    {
        MethodSymbol* read_method = NULL;
        AstName* name = left_hand_side -> NameCast();
        AstFieldAccess* field_access = left_hand_side -> FieldAccessCast();
        if (name)
        {
            if (name -> resolution_opt)
                read_method =
                    name -> resolution_opt -> symbol -> MethodCast();
        }
        else if (field_access)
        {
            if (field_access -> resolution_opt)
                read_method =
                    field_access -> resolution_opt -> symbol -> MethodCast();
        }

        if (read_method)
            assignment_expression -> write_method = read_method ->
                containing_type -> GetWriteAccessFromReadAccess(read_method);
    }

    if (assignment_expression -> Tag() ==
        AstAssignmentExpression::SIMPLE_EQUAL)
    {
        if (left_type != right_type)
        {
            if (CanAssignmentConvert(left_type,
                                     assignment_expression -> expression))
            {
                assignment_expression -> expression =
                    ConvertToType(assignment_expression -> expression,
                                  left_type);
            }
            else if (assignment_expression -> expression -> IsConstant() &&
                     control.IsSimpleIntegerValueType(left_type) &&
                     control.IsSimpleIntegerValueType(right_type))
            {
                if (left_type == control.byte_type)
                    ReportSemError(SemanticError::INVALID_BYTE_VALUE,
                                   assignment_expression -> expression);
                else if (left_type == control.short_type)
                    ReportSemError(SemanticError::INVALID_SHORT_VALUE,
                                   assignment_expression -> expression);
                else
                {
                    assert(left_type == control.char_type);
                    ReportSemError(SemanticError::INVALID_CHARACTER_VALUE,
                                   assignment_expression -> expression);
                }
                assignment_expression -> symbol = control.no_type;
            }
            else
            {
                ReportSemError(SemanticError::INCOMPATIBLE_TYPE_FOR_ASSIGNMENT,
                               assignment_expression,
                               left_type -> ContainingPackageName(),
                               left_type -> ExternalName(),
                               right_type -> ContainingPackageName(),
                               right_type -> ExternalName());
                assignment_expression -> symbol = control.no_type;
            }
        }
        return;
    }

    //
    // In JLS 2, it states that the only reference type on the left can
    // be String, for +=.  However, some compilers accept any type on the left
    // that can be assigned a String, provided the right side is a String.
    // In the process, that means an array access could then throw an
    // ArrayStoreException when the left type is not String.
    //
    // TODO: Get the definative answer from Sun which behavior is correct
    //
    if (left_type == control.String() &&
        (assignment_expression -> Tag() ==
         AstAssignmentExpression::PLUS_EQUAL))
    {
        if (right_type != control.String())
        {
            if (right_type == control.void_type)
            {
                 ReportSemError(SemanticError::VOID_TO_STRING,
                                assignment_expression -> expression);
                 assignment_expression -> symbol = control.no_type;
            }
            else
            {
                assignment_expression -> expression -> value =
                    CastValue(control.String(),
                              assignment_expression -> expression);
                if (assignment_expression -> expression -> IsConstant())
                {
                    assignment_expression -> expression -> symbol =
                        control.String();
                }
            }
        }
        return;
    }

    switch (assignment_expression -> Tag())
    {
        case AstAssignmentExpression::PLUS_EQUAL:
        case AstAssignmentExpression::STAR_EQUAL:
        case AstAssignmentExpression::MINUS_EQUAL:
            BinaryNumericPromotion(assignment_expression);
            break;
        case AstAssignmentExpression::SLASH_EQUAL:
        case AstAssignmentExpression::MOD_EQUAL:
            BinaryNumericPromotion(assignment_expression);
            {
                AstExpression* right_expression =
                    assignment_expression -> expression;
                if (right_expression -> IsConstant())
                {
                    //
                    // If the type of the expression is integral and the right
                    // hand side is constant 0 then issue an error message.
                    //
                    if ((right_expression -> Type() == control.int_type &&
                         DYNAMIC_CAST<IntLiteralValue*>
                         (right_expression -> value) -> value == 0) ||
                        (right_expression -> Type() == control.long_type &&
                         DYNAMIC_CAST<LongLiteralValue*>
                         (right_expression -> value) -> value == 0))
                    {
                        //
                        // This will guarantee a runtime exception, but the
                        // clarifications to JLS2 insist it is legal code.
                        //
                        ReportSemError(SemanticError::ZERO_DIVIDE_CAUTION,
                                       assignment_expression);
                    }
                }
            }
            break;
        case AstAssignmentExpression::LEFT_SHIFT_EQUAL:
        case AstAssignmentExpression::RIGHT_SHIFT_EQUAL:
        case AstAssignmentExpression::UNSIGNED_RIGHT_SHIFT_EQUAL:
            assignment_expression -> left_hand_side
                = PromoteUnaryNumericExpression(left_hand_side);
            if (! control.IsIntegral(left_type))
            {
                if (assignment_expression -> left_hand_side -> symbol !=
                    control.no_type)
                {
                    ReportSemError(SemanticError::TYPE_NOT_INTEGRAL,
                                   assignment_expression -> left_hand_side,
                                   left_type -> ContainingPackageName(),
                                   left_type -> ExternalName());
                }
                assignment_expression -> symbol = control.no_type;
            }
            //
            // This call captures both unary numeric conversion (widening) of
            // byte, char, or short, and narrowing of long, since the bytecode
            // requires an int shift amount.
            //
            if (! control.IsIntegral(right_type))
            {
                ReportSemError(SemanticError::TYPE_NOT_INTEGRAL,
                               assignment_expression -> expression,
                               right_type -> ContainingPackageName(),
                               right_type -> ExternalName());
                assignment_expression -> symbol = control.no_type;
            }
            assignment_expression -> expression =
                ConvertToType(assignment_expression -> expression,
                              control.int_type);
            ProcessShiftCount(left_type, assignment_expression -> expression);
            break;
        case AstAssignmentExpression::AND_EQUAL:
        case AstAssignmentExpression::XOR_EQUAL:
        case AstAssignmentExpression::IOR_EQUAL:
            // Java 5: Unbox Boolean to boolean for RHS
            if (right_type == control.Boolean())
            {
                assignment_expression -> expression =
                    ConvertToType(assignment_expression -> expression, control.boolean_type);
                right_type = control.boolean_type;
            }
            if (left_type == control.boolean_type)
            {
                if (right_type != control.boolean_type)
                {
                    ReportSemError(SemanticError::TYPE_NOT_BOOLEAN,
                                   assignment_expression -> expression,
                                   right_type -> ContainingPackageName(),
                                   right_type -> ExternalName());
                    assignment_expression -> symbol = control.no_type;
                }
            }
            else
            {
                // Java 5: Unbox numeric wrapper types before checking
                if (control.option.source >= JopaOption::SDK1_5)
                {
                    TypeSymbol* unboxed_right = right_type -> UnboxedType(control);
                    if (unboxed_right && unboxed_right != right_type &&
                        control.IsIntegral(unboxed_right))
                    {
                        assignment_expression -> expression =
                            ConvertToType(assignment_expression -> expression, unboxed_right);
                        right_type = unboxed_right;
                    }
                }
                if (! control.IsIntegral(left_type))
                {
                    ReportSemError(SemanticError::TYPE_NOT_INTEGRAL,
                                   left_hand_side,
                                   left_type -> ContainingPackageName(),
                                   left_type -> ExternalName());
                    assignment_expression -> symbol = control.no_type;
                }
                if (! control.IsIntegral(right_type))
                {
                    ReportSemError(SemanticError::TYPE_NOT_INTEGRAL,
                                   assignment_expression -> expression,
                                   right_type -> ContainingPackageName(),
                                   right_type -> ExternalName());
                    assignment_expression -> symbol = control.no_type;
                }
                BinaryNumericPromotion(assignment_expression);
            }
            break;
        default:
            assert(false);
            break;
    }
}



} // Close namespace Jopa block
