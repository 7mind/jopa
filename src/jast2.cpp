#include "jast2.h"

namespace Jopa {
namespace jast2 {

//=============================================================================
// LOCATION
//=============================================================================

SourceLocation SourceLocation::fromJson(const nlohmann::json& j) {
    SourceLocation loc;
    if (j.contains("location") && j["location"].is_object()) {
        loc.line = j["location"].value("line", 0);
        loc.column = j["location"].value("column", 0);
    }
    return loc;
}

//=============================================================================
// NAME AND MODIFIERS
//=============================================================================

std::string Name::getFullName() const {
    if (base) {
        return base->getFullName() + "." + identifier;
    }
    return identifier;
}

std::unique_ptr<Name> Name::fromJson(const nlohmann::json& j, Node* parent) {
    auto name = std::make_unique<Name>();
    name->parent = parent;
    name->location = SourceLocation::fromJson(j);
    name->identifier = j.value("identifier", "");
    if (j.contains("base") && !j["base"].is_null()) {
        name->base = Name::fromJson(j["base"], name.get());
    }
    return name;
}

bool Modifiers::hasModifier(const std::string& mod) const {
    for (const auto& m : modifiers) {
        if (m.kind == "ModifierKeyword" && m.value == mod) {
            return true;
        }
    }
    return false;
}

std::unique_ptr<Modifiers> Modifiers::fromJson(const nlohmann::json& j, Node* parent) {
    auto mods = std::make_unique<Modifiers>();
    mods->parent = parent;
    mods->location = SourceLocation::fromJson(j);

    if (j.contains("modifiers") && j["modifiers"].is_array()) {
        for (const auto& m : j["modifiers"]) {
            Modifier mod;
            mod.kind = m.value("kind", "");
            mod.value = m.value("modifier", "");
            if (mod.value.empty()) {
                mod.value = m.value("name", "");
            }
            mods->modifiers.push_back(mod);
        }
    }
    return mods;
}

//=============================================================================
// TYPES
//=============================================================================

std::unique_ptr<Type> Type::fromJson(const nlohmann::json& j, Node* parent) {
    if (!j.is_object()) return nullptr;

    std::string kind = j.value("kind", "");

    if (kind == "PrimitiveType") {
        return PrimitiveType::fromJson(j, parent);
    } else if (kind == "ArrayType") {
        return ArrayType::fromJson(j, parent);
    } else if (kind == "TypeName") {
        return TypeName::fromJson(j, parent);
    } else if (kind == "Wildcard") {
        return WildcardType::fromJson(j, parent);
    }

    return nullptr;
}

std::unique_ptr<PrimitiveType> PrimitiveType::fromJson(const nlohmann::json& j, Node* parent) {
    auto type = std::make_unique<PrimitiveType>();
    type->parent = parent;
    type->location = SourceLocation::fromJson(j);
    type->type = j.value("type", "");
    return type;
}

std::unique_ptr<ArrayType> ArrayType::fromJson(const nlohmann::json& j, Node* parent) {
    auto type = std::make_unique<ArrayType>();
    type->parent = parent;
    type->location = SourceLocation::fromJson(j);

    if (j.contains("elementType")) {
        type->elementType = Type::fromJson(j["elementType"], type.get());
    }

    if (j.contains("brackets") && j["brackets"].is_object()) {
        type->dimensions = j["brackets"].value("dims", 1);
    }

    return type;
}

std::unique_ptr<TypeName> TypeName::fromJson(const nlohmann::json& j, Node* parent) {
    auto type = std::make_unique<TypeName>();
    type->parent = parent;
    type->location = SourceLocation::fromJson(j);

    if (j.contains("name")) {
        type->name = Name::fromJson(j["name"], type.get());
    }

    if (j.contains("base") && !j["base"].is_null()) {
        type->base = TypeName::fromJson(j["base"], type.get());
    }

    if (j.contains("typeArguments") && j["typeArguments"].is_array()) {
        for (const auto& arg : j["typeArguments"]) {
            auto typeArg = Type::fromJson(arg, type.get());
            if (typeArg) type->typeArguments.push_back(std::move(typeArg));
        }
    }

    return type;
}

std::unique_ptr<WildcardType> WildcardType::fromJson(const nlohmann::json& j, Node* parent) {
    auto type = std::make_unique<WildcardType>();
    type->parent = parent;
    type->location = SourceLocation::fromJson(j);

    std::string boundKindStr = j.value("bound_kind", "");
    if (boundKindStr == "extends") {
        type->boundKind = WildcardType::BoundKind::EXTENDS;
    } else if (boundKindStr == "super") {
        type->boundKind = WildcardType::BoundKind::SUPER;
    }

    if (j.contains("bound") && !j["bound"].is_null()) {
        type->bound = Type::fromJson(j["bound"], type.get());
    }

    return type;
}

std::unique_ptr<TypeParameter> TypeParameter::fromJson(const nlohmann::json& j, Node* parent) {
    auto param = std::make_unique<TypeParameter>();
    param->parent = parent;
    param->location = SourceLocation::fromJson(j);
    param->name = j.value("name", "");

    if (j.contains("bounds") && j["bounds"].is_array()) {
        for (const auto& bound : j["bounds"]) {
            auto typeName = TypeName::fromJson(bound, param.get());
            if (typeName) param->bounds.push_back(std::move(typeName));
        }
    }

    return param;
}

//=============================================================================
// EXPRESSIONS
//=============================================================================

std::unique_ptr<Expression> Expression::fromJson(const nlohmann::json& j, Node* parent) {
    if (!j.is_object()) return nullptr;

    std::string kind = j.value("kind", "");

    if (kind == "IntegerLiteral" || kind == "LongLiteral" ||
        kind == "FloatLiteral" || kind == "DoubleLiteral" ||
        kind == "CharacterLiteral" || kind == "StringLiteral" ||
        kind == "BooleanLiteral" || kind == "NullLiteral") {
        return LiteralExpression::fromJson(j, parent);
    } else if (kind == "NameExpression") {
        return NameExpression::fromJson(j, parent);
    } else if (kind == "ThisExpression") {
        return ThisExpression::fromJson(j, parent);
    } else if (kind == "SuperExpression") {
        return SuperExpression::fromJson(j, parent);
    } else if (kind == "ParenthesizedExpression") {
        return ParenthesizedExpression::fromJson(j, parent);
    } else if (kind == "BinaryExpression") {
        return BinaryExpression::fromJson(j, parent);
    } else if (kind == "PreUnaryExpression") {
        return UnaryExpression::fromJson(j, parent, true);
    } else if (kind == "PostUnaryExpression") {
        return UnaryExpression::fromJson(j, parent, false);
    } else if (kind == "AssignmentExpression") {
        return AssignmentExpression::fromJson(j, parent);
    } else if (kind == "ConditionalExpression") {
        return ConditionalExpression::fromJson(j, parent);
    } else if (kind == "InstanceofExpression") {
        return InstanceofExpression::fromJson(j, parent);
    } else if (kind == "CastExpression") {
        return CastExpression::fromJson(j, parent);
    } else if (kind == "FieldAccess") {
        return FieldAccessExpression::fromJson(j, parent);
    } else if (kind == "MethodInvocation") {
        return MethodInvocationExpression::fromJson(j, parent);
    } else if (kind == "ArrayAccess") {
        return ArrayAccessExpression::fromJson(j, parent);
    } else if (kind == "ClassCreationExpression") {
        return ClassCreationExpression::fromJson(j, parent);
    } else if (kind == "ArrayCreationExpression") {
        return ArrayCreationExpression::fromJson(j, parent);
    } else if (kind == "ArrayInitializer") {
        return ArrayInitializer::fromJson(j, parent);
    } else if (kind == "ClassLiteral") {
        return ClassLiteralExpression::fromJson(j, parent);
    }

    return nullptr;
}

std::unique_ptr<LiteralExpression> LiteralExpression::fromJson(const nlohmann::json& j, Node* parent) {
    auto expr = std::make_unique<LiteralExpression>();
    expr->parent = parent;
    expr->location = SourceLocation::fromJson(j);

    std::string kind = j.value("kind", "");
    if (kind.length() > 7 && kind.substr(kind.length() - 7) == "Literal") {
        expr->literalKind = kind.substr(0, kind.length() - 7);
    }
    expr->value = j.value("value", "");

    return expr;
}

std::unique_ptr<NameExpression> NameExpression::fromJson(const nlohmann::json& j, Node* parent) {
    auto expr = std::make_unique<NameExpression>();
    expr->parent = parent;
    expr->location = SourceLocation::fromJson(j);

    if (j.contains("name")) {
        expr->name = Name::fromJson(j["name"], expr.get());
    }

    return expr;
}

std::unique_ptr<ThisExpression> ThisExpression::fromJson(const nlohmann::json& j, Node* parent) {
    auto expr = std::make_unique<ThisExpression>();
    expr->parent = parent;
    expr->location = SourceLocation::fromJson(j);

    if (j.contains("qualifier") && !j["qualifier"].is_null()) {
        expr->qualifier = TypeName::fromJson(j["qualifier"], expr.get());
    }

    return expr;
}

std::unique_ptr<SuperExpression> SuperExpression::fromJson(const nlohmann::json& j, Node* parent) {
    auto expr = std::make_unique<SuperExpression>();
    expr->parent = parent;
    expr->location = SourceLocation::fromJson(j);

    if (j.contains("qualifier") && !j["qualifier"].is_null()) {
        expr->qualifier = TypeName::fromJson(j["qualifier"], expr.get());
    }

    return expr;
}

std::unique_ptr<ParenthesizedExpression> ParenthesizedExpression::fromJson(const nlohmann::json& j, Node* parent) {
    auto expr = std::make_unique<ParenthesizedExpression>();
    expr->parent = parent;
    expr->location = SourceLocation::fromJson(j);

    if (j.contains("expression")) {
        expr->expression = Expression::fromJson(j["expression"], expr.get());
    }

    return expr;
}

std::unique_ptr<BinaryExpression> BinaryExpression::fromJson(const nlohmann::json& j, Node* parent) {
    auto expr = std::make_unique<BinaryExpression>();
    expr->parent = parent;
    expr->location = SourceLocation::fromJson(j);
    expr->op = j.value("operator", "");

    if (j.contains("left")) {
        expr->left = Expression::fromJson(j["left"], expr.get());
    }
    if (j.contains("right")) {
        expr->right = Expression::fromJson(j["right"], expr.get());
    }

    return expr;
}

std::unique_ptr<UnaryExpression> UnaryExpression::fromJson(const nlohmann::json& j, Node* parent, bool prefix) {
    auto expr = std::make_unique<UnaryExpression>();
    expr->parent = parent;
    expr->location = SourceLocation::fromJson(j);
    expr->op = j.value("operator", "");
    expr->isPrefix = prefix;

    if (j.contains("operand")) {
        expr->operand = Expression::fromJson(j["operand"], expr.get());
    }

    return expr;
}

std::unique_ptr<AssignmentExpression> AssignmentExpression::fromJson(const nlohmann::json& j, Node* parent) {
    auto expr = std::make_unique<AssignmentExpression>();
    expr->parent = parent;
    expr->location = SourceLocation::fromJson(j);
    expr->op = j.value("operator", "=");

    if (j.contains("left")) {
        expr->left = Expression::fromJson(j["left"], expr.get());
    }
    if (j.contains("right")) {
        expr->right = Expression::fromJson(j["right"], expr.get());
    }

    return expr;
}

std::unique_ptr<ConditionalExpression> ConditionalExpression::fromJson(const nlohmann::json& j, Node* parent) {
    auto expr = std::make_unique<ConditionalExpression>();
    expr->parent = parent;
    expr->location = SourceLocation::fromJson(j);

    if (j.contains("condition")) {
        expr->condition = Expression::fromJson(j["condition"], expr.get());
    }
    if (j.contains("true_expression")) {
        expr->thenExpr = Expression::fromJson(j["true_expression"], expr.get());
    }
    if (j.contains("false_expression")) {
        expr->elseExpr = Expression::fromJson(j["false_expression"], expr.get());
    }

    return expr;
}

std::unique_ptr<InstanceofExpression> InstanceofExpression::fromJson(const nlohmann::json& j, Node* parent) {
    auto expr = std::make_unique<InstanceofExpression>();
    expr->parent = parent;
    expr->location = SourceLocation::fromJson(j);

    if (j.contains("expression")) {
        expr->expression = Expression::fromJson(j["expression"], expr.get());
    }
    if (j.contains("type")) {
        expr->type = Type::fromJson(j["type"], expr.get());
    }

    return expr;
}

std::unique_ptr<CastExpression> CastExpression::fromJson(const nlohmann::json& j, Node* parent) {
    auto expr = std::make_unique<CastExpression>();
    expr->parent = parent;
    expr->location = SourceLocation::fromJson(j);

    if (j.contains("type")) {
        expr->type = Type::fromJson(j["type"], expr.get());
    }
    if (j.contains("expression")) {
        expr->expression = Expression::fromJson(j["expression"], expr.get());
    }

    return expr;
}

std::unique_ptr<FieldAccessExpression> FieldAccessExpression::fromJson(const nlohmann::json& j, Node* parent) {
    auto expr = std::make_unique<FieldAccessExpression>();
    expr->parent = parent;
    expr->location = SourceLocation::fromJson(j);

    if (j.contains("base")) {
        expr->base = Expression::fromJson(j["base"], expr.get());
    }
    expr->identifier = j.value("identifier", "");

    return expr;
}

std::unique_ptr<MethodInvocationExpression> MethodInvocationExpression::fromJson(const nlohmann::json& j, Node* parent) {
    auto expr = std::make_unique<MethodInvocationExpression>();
    expr->parent = parent;
    expr->location = SourceLocation::fromJson(j);

    if (j.contains("base") && !j["base"].is_null()) {
        expr->base = Expression::fromJson(j["base"], expr.get());
    }
    expr->methodName = j.value("name", "");

    if (j.contains("typeArguments") && j["typeArguments"].is_array()) {
        for (const auto& arg : j["typeArguments"]) {
            auto type = Type::fromJson(arg, expr.get());
            if (type) expr->typeArguments.push_back(std::move(type));
        }
    }

    if (j.contains("arguments") && j["arguments"].is_array()) {
        for (const auto& arg : j["arguments"]) {
            auto argExpr = Expression::fromJson(arg, expr.get());
            if (argExpr) expr->arguments.push_back(std::move(argExpr));
        }
    }

    return expr;
}

std::unique_ptr<ArrayAccessExpression> ArrayAccessExpression::fromJson(const nlohmann::json& j, Node* parent) {
    auto expr = std::make_unique<ArrayAccessExpression>();
    expr->parent = parent;
    expr->location = SourceLocation::fromJson(j);

    if (j.contains("array")) {
        expr->array = Expression::fromJson(j["array"], expr.get());
    }
    if (j.contains("index")) {
        expr->index = Expression::fromJson(j["index"], expr.get());
    }

    return expr;
}

void ClassCreationExpression::visitChildren(Visitor* v) {
    if (base) base->accept(v);
    if (type) type->accept(v);
    for (auto& arg : typeArguments) if (arg) arg->accept(v);
    for (auto& arg : arguments) if (arg) arg->accept(v);
    if (classBody) classBody->accept(v);
}

std::unique_ptr<ClassCreationExpression> ClassCreationExpression::fromJson(const nlohmann::json& j, Node* parent) {
    auto expr = std::make_unique<ClassCreationExpression>();
    expr->parent = parent;
    expr->location = SourceLocation::fromJson(j);

    if (j.contains("base") && !j["base"].is_null()) {
        expr->base = Expression::fromJson(j["base"], expr.get());
    }
    if (j.contains("type")) {
        expr->type = TypeName::fromJson(j["type"], expr.get());
    }

    if (j.contains("typeArguments") && j["typeArguments"].is_array()) {
        for (const auto& arg : j["typeArguments"]) {
            auto type = Type::fromJson(arg, expr.get());
            if (type) expr->typeArguments.push_back(std::move(type));
        }
    }

    if (j.contains("arguments") && j["arguments"].is_array()) {
        for (const auto& arg : j["arguments"]) {
            auto argExpr = Expression::fromJson(arg, expr.get());
            if (argExpr) expr->arguments.push_back(std::move(argExpr));
        }
    }

    if (j.contains("class_body") && !j["class_body"].is_null()) {
        expr->classBody = ClassBody::fromJson(j["class_body"], expr.get());
    }

    return expr;
}

void ArrayCreationExpression::visitChildren(Visitor* v) {
    if (elementType) elementType->accept(v);
    for (auto& dim : dimensionExprs) if (dim) dim->accept(v);
    if (initializer) initializer->accept(v);
}

std::unique_ptr<ArrayCreationExpression> ArrayCreationExpression::fromJson(const nlohmann::json& j, Node* parent) {
    auto expr = std::make_unique<ArrayCreationExpression>();
    expr->parent = parent;
    expr->location = SourceLocation::fromJson(j);

    if (j.contains("element_type")) {
        expr->elementType = Type::fromJson(j["element_type"], expr.get());
    }

    if (j.contains("dimensions") && j["dimensions"].is_array()) {
        for (const auto& dim : j["dimensions"]) {
            auto dimExpr = Expression::fromJson(dim, expr.get());
            if (dimExpr) expr->dimensionExprs.push_back(std::move(dimExpr));
        }
    }

    expr->extraDims = j.value("extra_dims", 0);

    if (j.contains("initializer") && !j["initializer"].is_null()) {
        expr->initializer = ArrayInitializer::fromJson(j["initializer"], expr.get());
    }

    return expr;
}

std::unique_ptr<ArrayInitializer> ArrayInitializer::fromJson(const nlohmann::json& j, Node* parent) {
    auto expr = std::make_unique<ArrayInitializer>();
    expr->parent = parent;
    expr->location = SourceLocation::fromJson(j);

    if (j.contains("elements") && j["elements"].is_array()) {
        for (const auto& elem : j["elements"]) {
            auto elemExpr = Expression::fromJson(elem, expr.get());
            if (elemExpr) expr->elements.push_back(std::move(elemExpr));
        }
    }

    return expr;
}

std::unique_ptr<ClassLiteralExpression> ClassLiteralExpression::fromJson(const nlohmann::json& j, Node* parent) {
    auto expr = std::make_unique<ClassLiteralExpression>();
    expr->parent = parent;
    expr->location = SourceLocation::fromJson(j);

    if (j.contains("type")) {
        expr->type = Type::fromJson(j["type"], expr.get());
    }

    return expr;
}

//=============================================================================
// STATEMENTS
//=============================================================================

std::unique_ptr<Statement> Statement::fromJson(const nlohmann::json& j, Node* parent) {
    if (!j.is_object()) return nullptr;

    std::string kind = j.value("kind", "");

    if (kind == "Block") {
        return BlockStatement::fromJson(j, parent);
    } else if (kind == "EmptyStatement") {
        return EmptyStatement::fromJson(j, parent);
    } else if (kind == "ExpressionStatement") {
        return ExpressionStatement::fromJson(j, parent);
    } else if (kind == "LocalVariableStatement") {
        return LocalVariableStatement::fromJson(j, parent);
    } else if (kind == "IfStatement") {
        return IfStatement::fromJson(j, parent);
    } else if (kind == "WhileStatement") {
        return WhileStatement::fromJson(j, parent);
    } else if (kind == "DoStatement") {
        return DoStatement::fromJson(j, parent);
    } else if (kind == "ForStatement") {
        return ForStatement::fromJson(j, parent);
    } else if (kind == "ForeachStatement") {
        return ForeachStatement::fromJson(j, parent);
    } else if (kind == "SwitchStatement") {
        return SwitchStatement::fromJson(j, parent);
    } else if (kind == "BreakStatement") {
        return BreakStatement::fromJson(j, parent);
    } else if (kind == "ContinueStatement") {
        return ContinueStatement::fromJson(j, parent);
    } else if (kind == "ReturnStatement") {
        return ReturnStatement::fromJson(j, parent);
    } else if (kind == "ThrowStatement") {
        return ThrowStatement::fromJson(j, parent);
    } else if (kind == "SynchronizedStatement") {
        return SynchronizedStatement::fromJson(j, parent);
    } else if (kind == "TryStatement") {
        return TryStatement::fromJson(j, parent);
    } else if (kind == "AssertStatement") {
        return AssertStatement::fromJson(j, parent);
    } else if (kind == "ThisCall") {
        return ThisCall::fromJson(j, parent);
    } else if (kind == "SuperCall") {
        return SuperCall::fromJson(j, parent);
    } else if (kind == "LocalClassStatement") {
        return LocalClassStatement::fromJson(j, parent);
    } else if (kind == "LabeledStatement") {
        return LabeledStatement::fromJson(j, parent);
    }

    return nullptr;
}

std::unique_ptr<BlockStatement> BlockStatement::fromJson(const nlohmann::json& j, Node* parent) {
    auto stmt = std::make_unique<BlockStatement>();
    stmt->parent = parent;
    stmt->location = SourceLocation::fromJson(j);

    if (j.contains("statements") && j["statements"].is_array()) {
        for (const auto& s : j["statements"]) {
            auto childStmt = Statement::fromJson(s, stmt.get());
            if (childStmt) stmt->statements.push_back(std::move(childStmt));
        }
    }

    return stmt;
}

std::unique_ptr<EmptyStatement> EmptyStatement::fromJson(const nlohmann::json& j, Node* parent) {
    auto stmt = std::make_unique<EmptyStatement>();
    stmt->parent = parent;
    stmt->location = SourceLocation::fromJson(j);
    return stmt;
}

std::unique_ptr<ExpressionStatement> ExpressionStatement::fromJson(const nlohmann::json& j, Node* parent) {
    auto stmt = std::make_unique<ExpressionStatement>();
    stmt->parent = parent;
    stmt->location = SourceLocation::fromJson(j);

    if (j.contains("expression")) {
        stmt->expression = Expression::fromJson(j["expression"], stmt.get());
    }

    return stmt;
}

std::unique_ptr<VariableDeclarator> VariableDeclarator::fromJson(const nlohmann::json& j, Node* parent) {
    auto decl = std::make_unique<VariableDeclarator>();
    decl->parent = parent;
    decl->location = SourceLocation::fromJson(j);

    if (j.contains("name") && j["name"].is_object()) {
        decl->name = j["name"].value("name", "");
    }
    decl->extraDims = j.value("extra_dims", 0);

    if (j.contains("initializer") && !j["initializer"].is_null()) {
        decl->initializer = Expression::fromJson(j["initializer"], decl.get());
    }

    return decl;
}

std::unique_ptr<LocalVariableStatement> LocalVariableStatement::fromJson(const nlohmann::json& j, Node* parent) {
    auto stmt = std::make_unique<LocalVariableStatement>();
    stmt->parent = parent;
    stmt->location = SourceLocation::fromJson(j);

    if (j.contains("modifiers") && !j["modifiers"].is_null()) {
        stmt->modifiers = Modifiers::fromJson(j["modifiers"], stmt.get());
    }
    if (j.contains("type")) {
        stmt->type = Type::fromJson(j["type"], stmt.get());
    }

    if (j.contains("declarators") && j["declarators"].is_array()) {
        for (const auto& d : j["declarators"]) {
            auto decl = VariableDeclarator::fromJson(d, stmt.get());
            if (decl) stmt->declarators.push_back(std::move(decl));
        }
    }

    return stmt;
}

std::unique_ptr<IfStatement> IfStatement::fromJson(const nlohmann::json& j, Node* parent) {
    auto stmt = std::make_unique<IfStatement>();
    stmt->parent = parent;
    stmt->location = SourceLocation::fromJson(j);

    if (j.contains("condition")) {
        stmt->condition = Expression::fromJson(j["condition"], stmt.get());
    }
    if (j.contains("then_statement")) {
        stmt->thenStatement = Statement::fromJson(j["then_statement"], stmt.get());
    }
    if (j.contains("else_statement") && !j["else_statement"].is_null()) {
        stmt->elseStatement = Statement::fromJson(j["else_statement"], stmt.get());
    }

    return stmt;
}

std::unique_ptr<WhileStatement> WhileStatement::fromJson(const nlohmann::json& j, Node* parent) {
    auto stmt = std::make_unique<WhileStatement>();
    stmt->parent = parent;
    stmt->location = SourceLocation::fromJson(j);

    if (j.contains("condition")) {
        stmt->condition = Expression::fromJson(j["condition"], stmt.get());
    }
    if (j.contains("body")) {
        stmt->body = Statement::fromJson(j["body"], stmt.get());
    }

    return stmt;
}

std::unique_ptr<DoStatement> DoStatement::fromJson(const nlohmann::json& j, Node* parent) {
    auto stmt = std::make_unique<DoStatement>();
    stmt->parent = parent;
    stmt->location = SourceLocation::fromJson(j);

    if (j.contains("body")) {
        stmt->body = Statement::fromJson(j["body"], stmt.get());
    }
    if (j.contains("condition")) {
        stmt->condition = Expression::fromJson(j["condition"], stmt.get());
    }

    return stmt;
}

std::unique_ptr<ForStatement> ForStatement::fromJson(const nlohmann::json& j, Node* parent) {
    auto stmt = std::make_unique<ForStatement>();
    stmt->parent = parent;
    stmt->location = SourceLocation::fromJson(j);

    if (j.contains("init") && j["init"].is_array()) {
        for (const auto& i : j["init"]) {
            auto initStmt = Statement::fromJson(i, stmt.get());
            if (initStmt) stmt->init.push_back(std::move(initStmt));
        }
    }
    if (j.contains("condition") && !j["condition"].is_null()) {
        stmt->condition = Expression::fromJson(j["condition"], stmt.get());
    }
    if (j.contains("update") && j["update"].is_array()) {
        for (const auto& u : j["update"]) {
            auto updateExpr = Expression::fromJson(u, stmt.get());
            if (updateExpr) stmt->update.push_back(std::move(updateExpr));
        }
    }
    if (j.contains("body")) {
        stmt->body = Statement::fromJson(j["body"], stmt.get());
    }

    return stmt;
}

std::unique_ptr<ForeachStatement> ForeachStatement::fromJson(const nlohmann::json& j, Node* parent) {
    auto stmt = std::make_unique<ForeachStatement>();
    stmt->parent = parent;
    stmt->location = SourceLocation::fromJson(j);

    if (j.contains("modifiers") && !j["modifiers"].is_null()) {
        stmt->modifiers = Modifiers::fromJson(j["modifiers"], stmt.get());
    }
    if (j.contains("type")) {
        stmt->type = Type::fromJson(j["type"], stmt.get());
    }
    stmt->variableName = j.value("variable_name", "");
    if (j.contains("iterable")) {
        stmt->iterable = Expression::fromJson(j["iterable"], stmt.get());
    }
    if (j.contains("body")) {
        stmt->body = Statement::fromJson(j["body"], stmt.get());
    }

    return stmt;
}

void SwitchStatement::visitChildren(Visitor* v) {
    if (expression) expression->accept(v);
    for (auto& block : blocks) {
        for (auto& label : block.labels) {
            if (label.expression) label.expression->accept(v);
        }
        for (auto& stmt : block.statements) {
            if (stmt) stmt->accept(v);
        }
    }
}

std::unique_ptr<SwitchStatement> SwitchStatement::fromJson(const nlohmann::json& j, Node* parent) {
    auto stmt = std::make_unique<SwitchStatement>();
    stmt->parent = parent;
    stmt->location = SourceLocation::fromJson(j);

    if (j.contains("expression")) {
        stmt->expression = Expression::fromJson(j["expression"], stmt.get());
    }

    if (j.contains("blocks") && j["blocks"].is_array()) {
        for (const auto& b : j["blocks"]) {
            SwitchBlock block;
            if (b.contains("labels") && b["labels"].is_array()) {
                for (const auto& l : b["labels"]) {
                    SwitchLabel label;
                    label.isDefault = l.value("is_default", false);
                    if (l.contains("expression") && !l["expression"].is_null()) {
                        label.expression = Expression::fromJson(l["expression"], stmt.get());
                    }
                    block.labels.push_back(std::move(label));
                }
            }
            if (b.contains("statements") && b["statements"].is_array()) {
                for (const auto& s : b["statements"]) {
                    auto childStmt = Statement::fromJson(s, stmt.get());
                    if (childStmt) block.statements.push_back(std::move(childStmt));
                }
            }
            stmt->blocks.push_back(std::move(block));
        }
    }

    return stmt;
}

std::unique_ptr<BreakStatement> BreakStatement::fromJson(const nlohmann::json& j, Node* parent) {
    auto stmt = std::make_unique<BreakStatement>();
    stmt->parent = parent;
    stmt->location = SourceLocation::fromJson(j);

    if (j.contains("label") && !j["label"].is_null()) {
        stmt->label = j["label"].get<std::string>();
    }

    return stmt;
}

std::unique_ptr<ContinueStatement> ContinueStatement::fromJson(const nlohmann::json& j, Node* parent) {
    auto stmt = std::make_unique<ContinueStatement>();
    stmt->parent = parent;
    stmt->location = SourceLocation::fromJson(j);

    if (j.contains("label") && !j["label"].is_null()) {
        stmt->label = j["label"].get<std::string>();
    }

    return stmt;
}

std::unique_ptr<ReturnStatement> ReturnStatement::fromJson(const nlohmann::json& j, Node* parent) {
    auto stmt = std::make_unique<ReturnStatement>();
    stmt->parent = parent;
    stmt->location = SourceLocation::fromJson(j);

    if (j.contains("expression") && !j["expression"].is_null()) {
        stmt->expression = Expression::fromJson(j["expression"], stmt.get());
    }

    return stmt;
}

std::unique_ptr<ThrowStatement> ThrowStatement::fromJson(const nlohmann::json& j, Node* parent) {
    auto stmt = std::make_unique<ThrowStatement>();
    stmt->parent = parent;
    stmt->location = SourceLocation::fromJson(j);

    if (j.contains("expression")) {
        stmt->expression = Expression::fromJson(j["expression"], stmt.get());
    }

    return stmt;
}

std::unique_ptr<SynchronizedStatement> SynchronizedStatement::fromJson(const nlohmann::json& j, Node* parent) {
    auto stmt = std::make_unique<SynchronizedStatement>();
    stmt->parent = parent;
    stmt->location = SourceLocation::fromJson(j);

    if (j.contains("expression")) {
        stmt->expression = Expression::fromJson(j["expression"], stmt.get());
    }
    if (j.contains("body")) {
        stmt->body = BlockStatement::fromJson(j["body"], stmt.get());
    }

    return stmt;
}

void TryStatement::visitChildren(Visitor* v) {
    if (tryBlock) tryBlock->accept(v);
    for (auto& clause : catchClauses) {
        if (clause.modifiers) clause.modifiers->accept(v);
        for (auto& type : clause.types) if (type) type->accept(v);
        if (clause.block) clause.block->accept(v);
    }
    if (finallyBlock) finallyBlock->accept(v);
}

std::unique_ptr<TryStatement> TryStatement::fromJson(const nlohmann::json& j, Node* parent) {
    auto stmt = std::make_unique<TryStatement>();
    stmt->parent = parent;
    stmt->location = SourceLocation::fromJson(j);

    if (j.contains("try_block")) {
        stmt->tryBlock = BlockStatement::fromJson(j["try_block"], stmt.get());
    }

    if (j.contains("catch_clauses") && j["catch_clauses"].is_array()) {
        for (const auto& c : j["catch_clauses"]) {
            CatchClause clause;
            if (c.contains("modifiers") && !c["modifiers"].is_null()) {
                clause.modifiers = Modifiers::fromJson(c["modifiers"], stmt.get());
            }
            if (c.contains("type")) {
                auto type = Type::fromJson(c["type"], stmt.get());
                if (type) clause.types.push_back(std::move(type));
            }
            clause.variableName = c.value("variable_name", "");
            if (c.contains("block")) {
                clause.block = BlockStatement::fromJson(c["block"], stmt.get());
            }
            stmt->catchClauses.push_back(std::move(clause));
        }
    }

    if (j.contains("finally_block") && !j["finally_block"].is_null()) {
        stmt->finallyBlock = BlockStatement::fromJson(j["finally_block"], stmt.get());
    }

    return stmt;
}

std::unique_ptr<AssertStatement> AssertStatement::fromJson(const nlohmann::json& j, Node* parent) {
    auto stmt = std::make_unique<AssertStatement>();
    stmt->parent = parent;
    stmt->location = SourceLocation::fromJson(j);

    if (j.contains("condition")) {
        stmt->condition = Expression::fromJson(j["condition"], stmt.get());
    }
    if (j.contains("message") && !j["message"].is_null()) {
        stmt->message = Expression::fromJson(j["message"], stmt.get());
    }

    return stmt;
}

std::unique_ptr<ThisCall> ThisCall::fromJson(const nlohmann::json& j, Node* parent) {
    auto stmt = std::make_unique<ThisCall>();
    stmt->parent = parent;
    stmt->location = SourceLocation::fromJson(j);

    if (j.contains("typeArguments") && j["typeArguments"].is_array()) {
        for (const auto& arg : j["typeArguments"]) {
            auto type = Type::fromJson(arg, stmt.get());
            if (type) stmt->typeArguments.push_back(std::move(type));
        }
    }

    if (j.contains("arguments") && j["arguments"].is_array()) {
        for (const auto& arg : j["arguments"]) {
            auto argExpr = Expression::fromJson(arg, stmt.get());
            if (argExpr) stmt->arguments.push_back(std::move(argExpr));
        }
    }

    return stmt;
}

std::unique_ptr<SuperCall> SuperCall::fromJson(const nlohmann::json& j, Node* parent) {
    auto stmt = std::make_unique<SuperCall>();
    stmt->parent = parent;
    stmt->location = SourceLocation::fromJson(j);

    if (j.contains("base") && !j["base"].is_null()) {
        stmt->base = Expression::fromJson(j["base"], stmt.get());
    }

    if (j.contains("typeArguments") && j["typeArguments"].is_array()) {
        for (const auto& arg : j["typeArguments"]) {
            auto type = Type::fromJson(arg, stmt.get());
            if (type) stmt->typeArguments.push_back(std::move(type));
        }
    }

    if (j.contains("arguments") && j["arguments"].is_array()) {
        for (const auto& arg : j["arguments"]) {
            auto argExpr = Expression::fromJson(arg, stmt.get());
            if (argExpr) stmt->arguments.push_back(std::move(argExpr));
        }
    }

    return stmt;
}

void LocalClassStatement::visitChildren(Visitor* v) {
    if (declaration) declaration->accept(v);
}

std::unique_ptr<LocalClassStatement> LocalClassStatement::fromJson(const nlohmann::json& j, Node* parent) {
    auto stmt = std::make_unique<LocalClassStatement>();
    stmt->parent = parent;
    stmt->location = SourceLocation::fromJson(j);

    if (j.contains("declaration")) {
        stmt->declaration = ClassDeclaration::fromJson(j["declaration"], stmt.get());
    }

    return stmt;
}

std::unique_ptr<LabeledStatement> LabeledStatement::fromJson(const nlohmann::json& j, Node* parent) {
    auto stmt = std::make_unique<LabeledStatement>();
    stmt->parent = parent;
    stmt->location = SourceLocation::fromJson(j);

    stmt->label = j.value("label", "");
    if (j.contains("statement")) {
        stmt->statement = Statement::fromJson(j["statement"], stmt.get());
    }

    return stmt;
}

//=============================================================================
// DECLARATIONS
//=============================================================================

std::unique_ptr<Declaration> Declaration::fromJson(const nlohmann::json& j, Node* parent) {
    if (!j.is_object()) return nullptr;

    std::string kind = j.value("kind", "");

    if (kind == "FieldDeclaration") {
        return FieldDeclaration::fromJson(j, parent);
    } else if (kind == "MethodDeclaration") {
        return MethodDeclaration::fromJson(j, parent);
    } else if (kind == "ConstructorDeclaration") {
        return ConstructorDeclaration::fromJson(j, parent);
    } else if (kind == "InitializerDeclaration") {
        return InitializerDeclaration::fromJson(j, parent);
    } else if (kind == "ClassDeclaration") {
        return ClassDeclaration::fromJson(j, parent);
    } else if (kind == "EnumDeclaration") {
        return EnumDeclaration::fromJson(j, parent);
    } else if (kind == "InterfaceDeclaration") {
        return InterfaceDeclaration::fromJson(j, parent);
    } else if (kind == "AnnotationDeclaration") {
        return AnnotationDeclaration::fromJson(j, parent);
    } else if (kind == "EmptyDeclaration") {
        return EmptyDeclaration::fromJson(j, parent);
    }

    return nullptr;
}

std::unique_ptr<FormalParameter> FormalParameter::fromJson(const nlohmann::json& j, Node* parent) {
    auto param = std::make_unique<FormalParameter>();
    param->parent = parent;
    param->location = SourceLocation::fromJson(j);

    if (j.contains("modifiers") && !j["modifiers"].is_null()) {
        param->modifiers = Modifiers::fromJson(j["modifiers"], param.get());
    }
    if (j.contains("type")) {
        param->type = Type::fromJson(j["type"], param.get());
    }

    if (j.contains("declarator") && j["declarator"].is_object()) {
        auto& decl = j["declarator"];
        if (decl.contains("name") && decl["name"].is_object()) {
            param->name = decl["name"].value("name", "");
        }
        param->extraDims = decl.value("extra_dims", 0);
    }

    param->isVarargs = j.value("is_vararg", false);

    return param;
}

std::unique_ptr<MethodBody> MethodBody::fromJson(const nlohmann::json& j, Node* parent) {
    auto body = std::make_unique<MethodBody>();
    body->parent = parent;
    body->location = SourceLocation::fromJson(j);

    if (j.contains("explicit_constructor") && !j["explicit_constructor"].is_null()) {
        body->explicitConstructorCall = Statement::fromJson(j["explicit_constructor"], body.get());
    }

    if (j.contains("statements") && j["statements"].is_array()) {
        for (const auto& s : j["statements"]) {
            auto stmt = Statement::fromJson(s, body.get());
            if (stmt) body->statements.push_back(std::move(stmt));
        }
    }

    return body;
}

std::unique_ptr<FieldDeclaration> FieldDeclaration::fromJson(const nlohmann::json& j, Node* parent) {
    auto decl = std::make_unique<FieldDeclaration>();
    decl->parent = parent;
    decl->location = SourceLocation::fromJson(j);

    if (j.contains("modifiers") && !j["modifiers"].is_null()) {
        decl->modifiers = Modifiers::fromJson(j["modifiers"], decl.get());
    }
    if (j.contains("type")) {
        decl->type = Type::fromJson(j["type"], decl.get());
    }

    if (j.contains("declarators") && j["declarators"].is_array()) {
        for (const auto& d : j["declarators"]) {
            auto varDecl = VariableDeclarator::fromJson(d, decl.get());
            if (varDecl) decl->declarators.push_back(std::move(varDecl));
        }
    }

    return decl;
}

std::unique_ptr<MethodDeclaration> MethodDeclaration::fromJson(const nlohmann::json& j, Node* parent) {
    auto decl = std::make_unique<MethodDeclaration>();
    decl->parent = parent;
    decl->location = SourceLocation::fromJson(j);

    if (j.contains("modifiers") && !j["modifiers"].is_null()) {
        decl->modifiers = Modifiers::fromJson(j["modifiers"], decl.get());
    }

    if (j.contains("type_parameters") && j["type_parameters"].is_array()) {
        for (const auto& tp : j["type_parameters"]) {
            auto param = TypeParameter::fromJson(tp, decl.get());
            if (param) decl->typeParameters.push_back(std::move(param));
        }
    }

    if (j.contains("return_type")) {
        decl->returnType = Type::fromJson(j["return_type"], decl.get());
    }

    if (j.contains("declarator") && j["declarator"].is_object()) {
        decl->name = j["declarator"].value("name", "");

        if (j["declarator"].contains("parameters") && j["declarator"]["parameters"].is_array()) {
            for (const auto& p : j["declarator"]["parameters"]) {
                auto param = FormalParameter::fromJson(p, decl.get());
                if (param) decl->parameters.push_back(std::move(param));
            }
        }
    }

    if (j.contains("throws") && j["throws"].is_array()) {
        for (const auto& t : j["throws"]) {
            auto typeName = TypeName::fromJson(t, decl.get());
            if (typeName) decl->throwsTypes.push_back(std::move(typeName));
        }
    }

    if (j.contains("body") && !j["body"].is_null()) {
        decl->body = MethodBody::fromJson(j["body"], decl.get());
    }

    if (j.contains("default_value") && !j["default_value"].is_null()) {
        decl->defaultValue = Expression::fromJson(j["default_value"], decl.get());
    }

    return decl;
}

std::unique_ptr<ConstructorDeclaration> ConstructorDeclaration::fromJson(const nlohmann::json& j, Node* parent) {
    auto decl = std::make_unique<ConstructorDeclaration>();
    decl->parent = parent;
    decl->location = SourceLocation::fromJson(j);

    if (j.contains("modifiers") && !j["modifiers"].is_null()) {
        decl->modifiers = Modifiers::fromJson(j["modifiers"], decl.get());
    }

    if (j.contains("type_parameters") && j["type_parameters"].is_array()) {
        for (const auto& tp : j["type_parameters"]) {
            auto param = TypeParameter::fromJson(tp, decl.get());
            if (param) decl->typeParameters.push_back(std::move(param));
        }
    }

    if (j.contains("declarator") && j["declarator"].is_object()) {
        decl->name = j["declarator"].value("name", "");

        if (j["declarator"].contains("parameters") && j["declarator"]["parameters"].is_array()) {
            for (const auto& p : j["declarator"]["parameters"]) {
                auto param = FormalParameter::fromJson(p, decl.get());
                if (param) decl->parameters.push_back(std::move(param));
            }
        }
    }

    if (j.contains("throws") && j["throws"].is_array()) {
        for (const auto& t : j["throws"]) {
            auto typeName = TypeName::fromJson(t, decl.get());
            if (typeName) decl->throwsTypes.push_back(std::move(typeName));
        }
    }

    if (j.contains("body") && !j["body"].is_null()) {
        decl->body = MethodBody::fromJson(j["body"], decl.get());
    }

    return decl;
}

std::unique_ptr<InitializerDeclaration> InitializerDeclaration::fromJson(const nlohmann::json& j, Node* parent) {
    auto decl = std::make_unique<InitializerDeclaration>();
    decl->parent = parent;
    decl->location = SourceLocation::fromJson(j);

    decl->isStatic = j.value("is_static", false);

    if (j.contains("body") && !j["body"].is_null()) {
        decl->body = MethodBody::fromJson(j["body"], decl.get());
    }

    return decl;
}

std::unique_ptr<ClassBody> ClassBody::fromJson(const nlohmann::json& j, Node* parent) {
    auto body = std::make_unique<ClassBody>();
    body->parent = parent;
    body->location = SourceLocation::fromJson(j);

    if (j.contains("declarations") && j["declarations"].is_array()) {
        for (const auto& d : j["declarations"]) {
            auto decl = Declaration::fromJson(d, body.get());
            if (decl) body->declarations.push_back(std::move(decl));
        }
    }

    return body;
}

std::unique_ptr<ClassDeclaration> ClassDeclaration::fromJson(const nlohmann::json& j, Node* parent) {
    auto decl = std::make_unique<ClassDeclaration>();
    decl->parent = parent;
    decl->location = SourceLocation::fromJson(j);

    if (j.contains("modifiers") && !j["modifiers"].is_null()) {
        decl->modifiers = Modifiers::fromJson(j["modifiers"], decl.get());
    }

    decl->name = j.value("name", "");

    if (j.contains("type_parameters") && j["type_parameters"].is_array()) {
        for (const auto& tp : j["type_parameters"]) {
            auto param = TypeParameter::fromJson(tp, decl.get());
            if (param) decl->typeParameters.push_back(std::move(param));
        }
    }

    if (j.contains("superclass") && !j["superclass"].is_null()) {
        decl->superclass = TypeName::fromJson(j["superclass"], decl.get());
    }

    if (j.contains("interfaces") && j["interfaces"].is_array()) {
        for (const auto& iface : j["interfaces"]) {
            auto typeName = TypeName::fromJson(iface, decl.get());
            if (typeName) decl->interfaces.push_back(std::move(typeName));
        }
    }

    if (j.contains("body") && !j["body"].is_null()) {
        decl->body = ClassBody::fromJson(j["body"], decl.get());
    }

    return decl;
}

std::unique_ptr<EnumConstant> EnumConstant::fromJson(const nlohmann::json& j, Node* parent) {
    auto decl = std::make_unique<EnumConstant>();
    decl->parent = parent;
    decl->location = SourceLocation::fromJson(j);

    decl->name = j.value("name", "");

    if (j.contains("arguments") && j["arguments"].is_array()) {
        for (const auto& arg : j["arguments"]) {
            auto argExpr = Expression::fromJson(arg, decl.get());
            if (argExpr) decl->arguments.push_back(std::move(argExpr));
        }
    }

    if (j.contains("body") && !j["body"].is_null()) {
        decl->body = ClassBody::fromJson(j["body"], decl.get());
    }

    return decl;
}

std::unique_ptr<EnumDeclaration> EnumDeclaration::fromJson(const nlohmann::json& j, Node* parent) {
    auto decl = std::make_unique<EnumDeclaration>();
    decl->parent = parent;
    decl->location = SourceLocation::fromJson(j);

    if (j.contains("modifiers") && !j["modifiers"].is_null()) {
        decl->modifiers = Modifiers::fromJson(j["modifiers"], decl.get());
    }

    decl->name = j.value("name", "");

    if (j.contains("interfaces") && j["interfaces"].is_array()) {
        for (const auto& iface : j["interfaces"]) {
            auto typeName = TypeName::fromJson(iface, decl.get());
            if (typeName) decl->interfaces.push_back(std::move(typeName));
        }
    }

    if (j.contains("constants") && j["constants"].is_array()) {
        for (const auto& c : j["constants"]) {
            auto constant = EnumConstant::fromJson(c, decl.get());
            if (constant) decl->constants.push_back(std::move(constant));
        }
    }

    if (j.contains("body") && !j["body"].is_null()) {
        decl->body = ClassBody::fromJson(j["body"], decl.get());
    }

    return decl;
}

std::unique_ptr<InterfaceDeclaration> InterfaceDeclaration::fromJson(const nlohmann::json& j, Node* parent) {
    auto decl = std::make_unique<InterfaceDeclaration>();
    decl->parent = parent;
    decl->location = SourceLocation::fromJson(j);

    if (j.contains("modifiers") && !j["modifiers"].is_null()) {
        decl->modifiers = Modifiers::fromJson(j["modifiers"], decl.get());
    }

    decl->name = j.value("name", "");

    if (j.contains("type_parameters") && j["type_parameters"].is_array()) {
        for (const auto& tp : j["type_parameters"]) {
            auto param = TypeParameter::fromJson(tp, decl.get());
            if (param) decl->typeParameters.push_back(std::move(param));
        }
    }

    if (j.contains("extends") && j["extends"].is_array()) {
        for (const auto& ext : j["extends"]) {
            auto typeName = TypeName::fromJson(ext, decl.get());
            if (typeName) decl->extendsTypes.push_back(std::move(typeName));
        }
    }

    if (j.contains("body") && !j["body"].is_null()) {
        decl->body = ClassBody::fromJson(j["body"], decl.get());
    }

    return decl;
}

std::unique_ptr<AnnotationDeclaration> AnnotationDeclaration::fromJson(const nlohmann::json& j, Node* parent) {
    auto decl = std::make_unique<AnnotationDeclaration>();
    decl->parent = parent;
    decl->location = SourceLocation::fromJson(j);

    if (j.contains("modifiers") && !j["modifiers"].is_null()) {
        decl->modifiers = Modifiers::fromJson(j["modifiers"], decl.get());
    }

    decl->name = j.value("name", "");

    if (j.contains("body") && !j["body"].is_null()) {
        decl->body = ClassBody::fromJson(j["body"], decl.get());
    }

    return decl;
}

std::unique_ptr<EmptyDeclaration> EmptyDeclaration::fromJson(const nlohmann::json& j, Node* parent) {
    auto decl = std::make_unique<EmptyDeclaration>();
    decl->parent = parent;
    decl->location = SourceLocation::fromJson(j);
    return decl;
}

//=============================================================================
// TOP-LEVEL
//=============================================================================

std::unique_ptr<PackageDeclaration> PackageDeclaration::fromJson(const nlohmann::json& j, Node* parent) {
    auto decl = std::make_unique<PackageDeclaration>();
    decl->parent = parent;
    decl->location = SourceLocation::fromJson(j);

    if (j.contains("modifiers") && !j["modifiers"].is_null()) {
        decl->modifiers = Modifiers::fromJson(j["modifiers"], decl.get());
    }

    if (j.contains("name")) {
        decl->name = Name::fromJson(j["name"], decl.get());
    }

    return decl;
}

std::unique_ptr<ImportDeclaration> ImportDeclaration::fromJson(const nlohmann::json& j, Node* parent) {
    auto decl = std::make_unique<ImportDeclaration>();
    decl->parent = parent;
    decl->location = SourceLocation::fromJson(j);

    if (j.contains("name")) {
        decl->name = Name::fromJson(j["name"], decl.get());
    }

    decl->isStatic = j.value("is_static", false);
    decl->isOnDemand = j.value("is_on_demand", false);

    return decl;
}

std::unique_ptr<CompilationUnit> CompilationUnit::fromJson(const nlohmann::json& j, Node* parent) {
    auto unit = std::make_unique<CompilationUnit>();
    unit->parent = parent;
    unit->location = SourceLocation::fromJson(j);

    if (j.contains("package") && !j["package"].is_null()) {
        unit->packageDecl = PackageDeclaration::fromJson(j["package"], unit.get());
    }

    if (j.contains("imports") && j["imports"].is_array()) {
        for (const auto& imp : j["imports"]) {
            auto importDecl = ImportDeclaration::fromJson(imp, unit.get());
            if (importDecl) unit->imports.push_back(std::move(importDecl));
        }
    }

    if (j.contains("types") && j["types"].is_array()) {
        for (const auto& t : j["types"]) {
            auto typeDecl = Declaration::fromJson(t, unit.get());
            if (typeDecl) unit->types.push_back(std::move(typeDecl));
        }
    }

    return unit;
}

} // namespace jast2
} // namespace Jopa
