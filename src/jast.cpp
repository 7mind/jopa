#include "jast.h"
#include <stdexcept>

namespace Jopa {
namespace jast {

// Location implementation
Location Location::fromJson(const nlohmann::json& j) {
    Location loc;
    if (j.is_object()) {
        if (j.contains("line")) loc.line = j["line"].get<int>();
        if (j.contains("column")) loc.column = j["column"].get<int>();
    }
    return loc;
}

nlohmann::json Location::toJson() const {
    return {{"line", line}, {"column", column}};
}

// Helper to get location from JSON if present
static Location getLocation(const nlohmann::json& j) {
    if (j.contains("location")) {
        return Location::fromJson(j["location"]);
    }
    return Location{};
}

// Name implementation
std::string Name::getFullName() const {
    if (base) {
        return base->getFullName() + "." + identifier;
    }
    return identifier;
}

std::unique_ptr<Name> Name::fromJson(const nlohmann::json& j) {
    if (j.is_null()) return nullptr;
    auto name = std::make_unique<Name>();
    name->location = getLocation(j);
    name->identifier = j.value("identifier", "");
    if (j.contains("base") && !j["base"].is_null()) {
        name->base = Name::fromJson(j["base"]);
    }
    return name;
}

// Modifiers implementation
bool Modifiers::hasModifier(const std::string& mod) const {
    for (const auto& m : modifiers) {
        if (m.kind == "keyword" && m.value == mod) return true;
    }
    return false;
}

std::unique_ptr<Modifiers> Modifiers::fromJson(const nlohmann::json& j) {
    if (j.is_null()) return nullptr;
    auto mods = std::make_unique<Modifiers>();
    mods->location = getLocation(j);
    if (j.contains("modifiers") && j["modifiers"].is_array()) {
        for (const auto& m : j["modifiers"]) {
            Modifier mod;
            std::string kind = m.value("kind", "");
            if (kind == "ModifierKeyword") {
                mod.kind = "keyword";
                mod.value = m.value("modifier", "");
            } else if (kind == "Annotation") {
                mod.kind = "annotation";
                if (m.contains("name")) {
                    auto name = Name::fromJson(m["name"]);
                    mod.value = name ? name->getFullName() : "";
                }
            }
            mods->modifiers.push_back(mod);
        }
    }
    return mods;
}

// Type implementations
std::unique_ptr<Type> Type::fromJson(const nlohmann::json& j) {
    if (j.is_null()) return nullptr;
    std::string kind = j.value("kind", "");

    if (kind == "PrimitiveType") {
        return PrimitiveType::fromJson(j);
    } else if (kind == "ArrayType") {
        return ArrayType::fromJson(j);
    } else if (kind == "TypeName") {
        return TypeName::fromJson(j);
    } else if (kind == "Wildcard") {
        return WildcardType::fromJson(j);
    }
    return nullptr;
}

std::unique_ptr<PrimitiveType> PrimitiveType::fromJson(const nlohmann::json& j) {
    auto type = std::make_unique<PrimitiveType>();
    type->location = getLocation(j);
    type->type = j.value("type", "");
    return type;
}

std::unique_ptr<ArrayType> ArrayType::fromJson(const nlohmann::json& j) {
    auto type = std::make_unique<ArrayType>();
    type->location = getLocation(j);
    if (j.contains("elementType")) {
        type->elementType = Type::fromJson(j["elementType"]);
    }
    if (j.contains("brackets") && j["brackets"].contains("dims")) {
        type->dimensions = j["brackets"]["dims"].get<int>();
    }
    return type;
}

std::unique_ptr<TypeName> TypeName::fromJson(const nlohmann::json& j) {
    auto type = std::make_unique<TypeName>();
    type->location = getLocation(j);
    if (j.contains("name")) {
        type->name = Name::fromJson(j["name"]);
    }
    if (j.contains("base") && !j["base"].is_null()) {
        type->base = TypeName::fromJson(j["base"]);
    }
    if (j.contains("type_arguments") && j["type_arguments"].contains("arguments")) {
        for (const auto& arg : j["type_arguments"]["arguments"]) {
            type->typeArguments.push_back(Type::fromJson(arg));
        }
    }
    return type;
}

std::unique_ptr<WildcardType> WildcardType::fromJson(const nlohmann::json& j) {
    auto type = std::make_unique<WildcardType>();
    type->location = getLocation(j);
    std::string boundKind = j.value("bound_kind", "");
    if (boundKind == "extends") {
        type->boundKind = WildcardType::BoundKind::EXTENDS;
    } else if (boundKind == "super") {
        type->boundKind = WildcardType::BoundKind::SUPER;
    }
    if (j.contains("bounds")) {
        type->bound = Type::fromJson(j["bounds"]);
    }
    return type;
}

std::unique_ptr<TypeParameter> TypeParameter::fromJson(const nlohmann::json& j) {
    auto param = std::make_unique<TypeParameter>();
    param->location = getLocation(j);
    param->name = j.value("name", "");
    if (j.contains("bounds") && j["bounds"].is_array()) {
        for (const auto& b : j["bounds"]) {
            param->bounds.push_back(TypeName::fromJson(b));
        }
    }
    return param;
}

// Expression implementations
std::unique_ptr<Expression> Expression::fromJson(const nlohmann::json& j) {
    if (j.is_null()) return nullptr;
    std::string kind = j.value("kind", "");

    if (kind == "IntegerLiteral" || kind == "LongLiteral" || kind == "FloatLiteral" ||
        kind == "DoubleLiteral" || kind == "StringLiteral" || kind == "CharacterLiteral" ||
        kind == "TrueLiteral" || kind == "FalseLiteral" || kind == "NullLiteral") {
        return LiteralExpression::fromJson(j);
    } else if (kind == "Name") {
        return NameExpression::fromJson(j);
    } else if (kind == "ThisExpression") {
        return ThisExpression::fromJson(j);
    } else if (kind == "SuperExpression") {
        return SuperExpression::fromJson(j);
    } else if (kind == "ParenthesizedExpression") {
        return ParenthesizedExpression::fromJson(j);
    } else if (kind == "BinaryExpression") {
        return BinaryExpression::fromJson(j);
    } else if (kind == "PreUnaryExpression") {
        return UnaryExpression::fromJson(j, true);
    } else if (kind == "PostUnaryExpression") {
        return UnaryExpression::fromJson(j, false);
    } else if (kind == "AssignmentExpression") {
        return AssignmentExpression::fromJson(j);
    } else if (kind == "ConditionalExpression") {
        return ConditionalExpression::fromJson(j);
    } else if (kind == "InstanceofExpression") {
        return InstanceofExpression::fromJson(j);
    } else if (kind == "CastExpression") {
        return CastExpression::fromJson(j);
    } else if (kind == "FieldAccess") {
        return FieldAccessExpression::fromJson(j);
    } else if (kind == "MethodInvocation") {
        return MethodInvocationExpression::fromJson(j);
    } else if (kind == "ArrayAccess") {
        return ArrayAccessExpression::fromJson(j);
    } else if (kind == "ClassCreationExpression") {
        return ClassCreationExpression::fromJson(j);
    } else if (kind == "ArrayCreationExpression") {
        return ArrayCreationExpression::fromJson(j);
    } else if (kind == "ArrayInitializer") {
        return ArrayInitializer::fromJson(j);
    } else if (kind == "ClassLiteral") {
        return ClassLiteralExpression::fromJson(j);
    }
    return nullptr;
}

std::unique_ptr<LiteralExpression> LiteralExpression::fromJson(const nlohmann::json& j) {
    auto expr = std::make_unique<LiteralExpression>();
    expr->location = getLocation(j);
    std::string kind = j.value("kind", "");
    if (kind == "IntegerLiteral") {
        expr->literalKind = "Integer";
    } else if (kind == "LongLiteral") {
        expr->literalKind = "Long";
    } else if (kind == "FloatLiteral") {
        expr->literalKind = "Float";
    } else if (kind == "DoubleLiteral") {
        expr->literalKind = "Double";
    } else if (kind == "StringLiteral") {
        expr->literalKind = "String";
    } else if (kind == "CharacterLiteral") {
        expr->literalKind = "Character";
    } else if (kind == "TrueLiteral") {
        expr->literalKind = "True";
        expr->value = "true";
    } else if (kind == "FalseLiteral") {
        expr->literalKind = "False";
        expr->value = "false";
    } else if (kind == "NullLiteral") {
        expr->literalKind = "Null";
        expr->value = "null";
    }
    if (j.contains("value")) {
        expr->value = j["value"].get<std::string>();
    }
    return expr;
}

std::unique_ptr<NameExpression> NameExpression::fromJson(const nlohmann::json& j) {
    auto expr = std::make_unique<NameExpression>();
    expr->location = getLocation(j);
    expr->name = Name::fromJson(j);
    return expr;
}

std::unique_ptr<ThisExpression> ThisExpression::fromJson(const nlohmann::json& j) {
    auto expr = std::make_unique<ThisExpression>();
    expr->location = getLocation(j);
    if (j.contains("base") && !j["base"].is_null()) {
        expr->qualifier = TypeName::fromJson(j["base"]);
    }
    return expr;
}

std::unique_ptr<SuperExpression> SuperExpression::fromJson(const nlohmann::json& j) {
    auto expr = std::make_unique<SuperExpression>();
    expr->location = getLocation(j);
    if (j.contains("base") && !j["base"].is_null()) {
        expr->qualifier = TypeName::fromJson(j["base"]);
    }
    return expr;
}

std::unique_ptr<ParenthesizedExpression> ParenthesizedExpression::fromJson(const nlohmann::json& j) {
    auto expr = std::make_unique<ParenthesizedExpression>();
    expr->location = getLocation(j);
    if (j.contains("expression")) {
        expr->expression = Expression::fromJson(j["expression"]);
    }
    return expr;
}

std::unique_ptr<BinaryExpression> BinaryExpression::fromJson(const nlohmann::json& j) {
    auto expr = std::make_unique<BinaryExpression>();
    expr->location = getLocation(j);
    expr->op = j.value("operator", "");
    if (j.contains("left")) {
        expr->left = Expression::fromJson(j["left"]);
    }
    if (j.contains("right")) {
        expr->right = Expression::fromJson(j["right"]);
    }
    return expr;
}

std::unique_ptr<UnaryExpression> UnaryExpression::fromJson(const nlohmann::json& j, bool prefix) {
    auto expr = std::make_unique<UnaryExpression>();
    expr->location = getLocation(j);
    expr->op = j.value("operator", "");
    expr->isPrefix = prefix;
    if (j.contains("expression")) {
        expr->operand = Expression::fromJson(j["expression"]);
    }
    return expr;
}

std::unique_ptr<AssignmentExpression> AssignmentExpression::fromJson(const nlohmann::json& j) {
    auto expr = std::make_unique<AssignmentExpression>();
    expr->location = getLocation(j);
    expr->op = j.value("operator", "");
    if (j.contains("left")) {
        expr->left = Expression::fromJson(j["left"]);
    }
    if (j.contains("right")) {
        expr->right = Expression::fromJson(j["right"]);
    }
    return expr;
}

std::unique_ptr<ConditionalExpression> ConditionalExpression::fromJson(const nlohmann::json& j) {
    auto expr = std::make_unique<ConditionalExpression>();
    expr->location = getLocation(j);
    if (j.contains("condition")) {
        expr->condition = Expression::fromJson(j["condition"]);
    }
    if (j.contains("true_expression")) {
        expr->thenExpr = Expression::fromJson(j["true_expression"]);
    }
    if (j.contains("false_expression")) {
        expr->elseExpr = Expression::fromJson(j["false_expression"]);
    }
    return expr;
}

std::unique_ptr<InstanceofExpression> InstanceofExpression::fromJson(const nlohmann::json& j) {
    auto expr = std::make_unique<InstanceofExpression>();
    expr->location = getLocation(j);
    if (j.contains("expression")) {
        expr->expression = Expression::fromJson(j["expression"]);
    }
    if (j.contains("type")) {
        expr->type = Type::fromJson(j["type"]);
    }
    return expr;
}

std::unique_ptr<CastExpression> CastExpression::fromJson(const nlohmann::json& j) {
    auto expr = std::make_unique<CastExpression>();
    expr->location = getLocation(j);
    if (j.contains("type")) {
        expr->type = Type::fromJson(j["type"]);
    }
    if (j.contains("expression")) {
        expr->expression = Expression::fromJson(j["expression"]);
    }
    return expr;
}

std::unique_ptr<FieldAccessExpression> FieldAccessExpression::fromJson(const nlohmann::json& j) {
    auto expr = std::make_unique<FieldAccessExpression>();
    expr->location = getLocation(j);
    if (j.contains("base")) {
        expr->base = Expression::fromJson(j["base"]);
    }
    expr->identifier = j.value("identifier", "");
    return expr;
}

std::unique_ptr<MethodInvocationExpression> MethodInvocationExpression::fromJson(const nlohmann::json& j) {
    auto expr = std::make_unique<MethodInvocationExpression>();
    expr->location = getLocation(j);
    if (j.contains("base") && !j["base"].is_null()) {
        expr->base = Expression::fromJson(j["base"]);
    }
    expr->methodName = j.value("identifier", "");
    if (j.contains("type_arguments") && j["type_arguments"].contains("arguments")) {
        for (const auto& arg : j["type_arguments"]["arguments"]) {
            expr->typeArguments.push_back(Type::fromJson(arg));
        }
    }
    if (j.contains("arguments") && j["arguments"].contains("arguments")) {
        for (const auto& arg : j["arguments"]["arguments"]) {
            expr->arguments.push_back(Expression::fromJson(arg));
        }
    }
    return expr;
}

std::unique_ptr<ArrayAccessExpression> ArrayAccessExpression::fromJson(const nlohmann::json& j) {
    auto expr = std::make_unique<ArrayAccessExpression>();
    expr->location = getLocation(j);
    if (j.contains("base")) {
        expr->array = Expression::fromJson(j["base"]);
    }
    if (j.contains("index")) {
        expr->index = Expression::fromJson(j["index"]);
    }
    return expr;
}

std::unique_ptr<ClassCreationExpression> ClassCreationExpression::fromJson(const nlohmann::json& j) {
    auto expr = std::make_unique<ClassCreationExpression>();
    expr->location = getLocation(j);
    if (j.contains("base") && !j["base"].is_null()) {
        expr->base = Expression::fromJson(j["base"]);
    }
    if (j.contains("type")) {
        expr->type = TypeName::fromJson(j["type"]);
    }
    if (j.contains("type_arguments") && j["type_arguments"].contains("arguments")) {
        for (const auto& arg : j["type_arguments"]["arguments"]) {
            expr->typeArguments.push_back(Type::fromJson(arg));
        }
    }
    if (j.contains("arguments") && j["arguments"].contains("arguments")) {
        for (const auto& arg : j["arguments"]["arguments"]) {
            expr->arguments.push_back(Expression::fromJson(arg));
        }
    }
    if (j.contains("body") && !j["body"].is_null()) {
        expr->classBody = ClassBody::fromJson(j["body"]);
    }
    return expr;
}

std::unique_ptr<ArrayCreationExpression> ArrayCreationExpression::fromJson(const nlohmann::json& j) {
    auto expr = std::make_unique<ArrayCreationExpression>();
    expr->location = getLocation(j);
    if (j.contains("type")) {
        expr->elementType = Type::fromJson(j["type"]);
    }
    if (j.contains("dim_exprs") && j["dim_exprs"].is_array()) {
        for (const auto& dim : j["dim_exprs"]) {
            if (dim.contains("expression")) {
                expr->dimensionExprs.push_back(Expression::fromJson(dim["expression"]));
            }
        }
    }
    if (j.contains("brackets") && j["brackets"].contains("dims")) {
        expr->extraDims = j["brackets"]["dims"].get<int>();
    }
    if (j.contains("initializer") && !j["initializer"].is_null()) {
        expr->initializer = ArrayInitializer::fromJson(j["initializer"]);
    }
    return expr;
}

std::unique_ptr<ArrayInitializer> ArrayInitializer::fromJson(const nlohmann::json& j) {
    auto expr = std::make_unique<ArrayInitializer>();
    expr->location = getLocation(j);
    if (j.contains("elements") && j["elements"].is_array()) {
        for (const auto& elem : j["elements"]) {
            expr->elements.push_back(Expression::fromJson(elem));
        }
    }
    return expr;
}

std::unique_ptr<ClassLiteralExpression> ClassLiteralExpression::fromJson(const nlohmann::json& j) {
    auto expr = std::make_unique<ClassLiteralExpression>();
    expr->location = getLocation(j);
    if (j.contains("type")) {
        expr->type = Type::fromJson(j["type"]);
    }
    return expr;
}

// Statement implementations
std::unique_ptr<Statement> Statement::fromJson(const nlohmann::json& j) {
    if (j.is_null()) return nullptr;
    std::string kind = j.value("kind", "");

    if (kind == "Block") {
        return BlockStatement::fromJson(j);
    } else if (kind == "EmptyStatement") {
        return EmptyStatement::fromJson(j);
    } else if (kind == "ExpressionStatement") {
        return ExpressionStatement::fromJson(j);
    } else if (kind == "LocalVariableStatement") {
        return LocalVariableStatement::fromJson(j);
    } else if (kind == "IfStatement") {
        return IfStatement::fromJson(j);
    } else if (kind == "WhileStatement") {
        return WhileStatement::fromJson(j);
    } else if (kind == "DoStatement") {
        return DoStatement::fromJson(j);
    } else if (kind == "ForStatement") {
        return ForStatement::fromJson(j);
    } else if (kind == "ForeachStatement") {
        return ForeachStatement::fromJson(j);
    } else if (kind == "SwitchStatement") {
        return SwitchStatement::fromJson(j);
    } else if (kind == "BreakStatement") {
        return BreakStatement::fromJson(j);
    } else if (kind == "ContinueStatement") {
        return ContinueStatement::fromJson(j);
    } else if (kind == "ReturnStatement") {
        return ReturnStatement::fromJson(j);
    } else if (kind == "ThrowStatement") {
        return ThrowStatement::fromJson(j);
    } else if (kind == "SynchronizedStatement") {
        return SynchronizedStatement::fromJson(j);
    } else if (kind == "TryStatement") {
        return TryStatement::fromJson(j);
    } else if (kind == "AssertStatement") {
        return AssertStatement::fromJson(j);
    } else if (kind == "ThisCall") {
        return ThisCall::fromJson(j);
    } else if (kind == "SuperCall") {
        return SuperCall::fromJson(j);
    } else if (kind == "LocalClassStatement") {
        return LocalClassStatement::fromJson(j);
    } else if (kind == "VariableDeclarator") {
        // Variable declarators can appear as statements in for-init
        auto stmt = std::make_unique<ExpressionStatement>();
        stmt->location = getLocation(j);
        return stmt;
    }
    return nullptr;
}

std::unique_ptr<BlockStatement> BlockStatement::fromJson(const nlohmann::json& j) {
    auto stmt = std::make_unique<BlockStatement>();
    stmt->location = getLocation(j);
    if (j.contains("statements") && j["statements"].is_array()) {
        for (const auto& s : j["statements"]) {
            auto child = Statement::fromJson(s);
            if (child) stmt->statements.push_back(std::move(child));
        }
    }
    return stmt;
}

std::unique_ptr<EmptyStatement> EmptyStatement::fromJson(const nlohmann::json& j) {
    auto stmt = std::make_unique<EmptyStatement>();
    stmt->location = getLocation(j);
    return stmt;
}

std::unique_ptr<ExpressionStatement> ExpressionStatement::fromJson(const nlohmann::json& j) {
    auto stmt = std::make_unique<ExpressionStatement>();
    stmt->location = getLocation(j);
    if (j.contains("expression")) {
        stmt->expression = Expression::fromJson(j["expression"]);
    }
    return stmt;
}

std::unique_ptr<VariableDeclarator> VariableDeclarator::fromJson(const nlohmann::json& j) {
    auto decl = std::make_unique<VariableDeclarator>();
    decl->location = getLocation(j);
    if (j.contains("name")) {
        if (j["name"].is_string()) {
            decl->name = j["name"].get<std::string>();
        } else if (j["name"].is_object() && j["name"].contains("name")) {
            decl->name = j["name"]["name"].get<std::string>();
        }
    }
    if (j.contains("initializer") && !j["initializer"].is_null()) {
        decl->initializer = Expression::fromJson(j["initializer"]);
    }
    return decl;
}

std::unique_ptr<LocalVariableStatement> LocalVariableStatement::fromJson(const nlohmann::json& j) {
    auto stmt = std::make_unique<LocalVariableStatement>();
    stmt->location = getLocation(j);
    if (j.contains("modifiers") && !j["modifiers"].is_null()) {
        stmt->modifiers = Modifiers::fromJson(j["modifiers"]);
    }
    if (j.contains("type")) {
        stmt->type = Type::fromJson(j["type"]);
    }
    if (j.contains("declarators") && j["declarators"].is_array()) {
        for (const auto& d : j["declarators"]) {
            stmt->declarators.push_back(VariableDeclarator::fromJson(d));
        }
    }
    return stmt;
}

std::unique_ptr<IfStatement> IfStatement::fromJson(const nlohmann::json& j) {
    auto stmt = std::make_unique<IfStatement>();
    stmt->location = getLocation(j);
    if (j.contains("condition")) {
        stmt->condition = Expression::fromJson(j["condition"]);
    }
    if (j.contains("then_statement")) {
        stmt->thenStatement = Statement::fromJson(j["then_statement"]);
    }
    if (j.contains("else_statement") && !j["else_statement"].is_null()) {
        stmt->elseStatement = Statement::fromJson(j["else_statement"]);
    }
    return stmt;
}

std::unique_ptr<WhileStatement> WhileStatement::fromJson(const nlohmann::json& j) {
    auto stmt = std::make_unique<WhileStatement>();
    stmt->location = getLocation(j);
    if (j.contains("condition")) {
        stmt->condition = Expression::fromJson(j["condition"]);
    }
    if (j.contains("body")) {
        stmt->body = Statement::fromJson(j["body"]);
    }
    return stmt;
}

std::unique_ptr<DoStatement> DoStatement::fromJson(const nlohmann::json& j) {
    auto stmt = std::make_unique<DoStatement>();
    stmt->location = getLocation(j);
    if (j.contains("body")) {
        stmt->body = Statement::fromJson(j["body"]);
    }
    if (j.contains("condition")) {
        stmt->condition = Expression::fromJson(j["condition"]);
    }
    return stmt;
}

std::unique_ptr<ForStatement> ForStatement::fromJson(const nlohmann::json& j) {
    auto stmt = std::make_unique<ForStatement>();
    stmt->location = getLocation(j);
    if (j.contains("init") && j["init"].is_array()) {
        for (const auto& s : j["init"]) {
            auto child = Statement::fromJson(s);
            if (child) stmt->init.push_back(std::move(child));
        }
    }
    if (j.contains("condition") && !j["condition"].is_null()) {
        stmt->condition = Expression::fromJson(j["condition"]);
    }
    if (j.contains("update") && j["update"].is_array()) {
        for (const auto& s : j["update"]) {
            if (s.contains("expression")) {
                stmt->update.push_back(Expression::fromJson(s["expression"]));
            }
        }
    }
    if (j.contains("body")) {
        stmt->body = Statement::fromJson(j["body"]);
    }
    return stmt;
}

std::unique_ptr<ForeachStatement> ForeachStatement::fromJson(const nlohmann::json& j) {
    auto stmt = std::make_unique<ForeachStatement>();
    stmt->location = getLocation(j);
    if (j.contains("parameter")) {
        auto param = FormalParameter::fromJson(j["parameter"]);
        if (param) {
            stmt->modifiers = std::move(param->modifiers);
            stmt->type = std::move(param->type);
            stmt->variableName = param->name;
        }
    }
    if (j.contains("expression")) {
        stmt->iterable = Expression::fromJson(j["expression"]);
    }
    if (j.contains("body")) {
        stmt->body = Statement::fromJson(j["body"]);
    }
    return stmt;
}

std::unique_ptr<SwitchStatement> SwitchStatement::fromJson(const nlohmann::json& j) {
    auto stmt = std::make_unique<SwitchStatement>();
    stmt->location = getLocation(j);
    if (j.contains("expression")) {
        stmt->expression = Expression::fromJson(j["expression"]);
    }
    if (j.contains("blocks") && j["blocks"].is_array()) {
        for (const auto& b : j["blocks"]) {
            SwitchStatement::SwitchBlock block;
            if (b.contains("labels") && b["labels"].is_array()) {
                for (const auto& l : b["labels"]) {
                    SwitchStatement::SwitchLabel label;
                    label.isDefault = l.value("is_default", false);
                    if (l.contains("expression") && !l["expression"].is_null()) {
                        label.expression = Expression::fromJson(l["expression"]);
                    }
                    block.labels.push_back(std::move(label));
                }
            }
            if (b.contains("statements") && b["statements"].is_array()) {
                for (const auto& s : b["statements"]) {
                    auto child = Statement::fromJson(s);
                    if (child) block.statements.push_back(std::move(child));
                }
            }
            stmt->blocks.push_back(std::move(block));
        }
    }
    return stmt;
}

std::unique_ptr<BreakStatement> BreakStatement::fromJson(const nlohmann::json& j) {
    auto stmt = std::make_unique<BreakStatement>();
    stmt->location = getLocation(j);
    if (j.contains("label") && !j["label"].is_null()) {
        stmt->label = j["label"].get<std::string>();
    }
    return stmt;
}

std::unique_ptr<ContinueStatement> ContinueStatement::fromJson(const nlohmann::json& j) {
    auto stmt = std::make_unique<ContinueStatement>();
    stmt->location = getLocation(j);
    if (j.contains("label") && !j["label"].is_null()) {
        stmt->label = j["label"].get<std::string>();
    }
    return stmt;
}

std::unique_ptr<ReturnStatement> ReturnStatement::fromJson(const nlohmann::json& j) {
    auto stmt = std::make_unique<ReturnStatement>();
    stmt->location = getLocation(j);
    if (j.contains("expression") && !j["expression"].is_null()) {
        stmt->expression = Expression::fromJson(j["expression"]);
    }
    return stmt;
}

std::unique_ptr<ThrowStatement> ThrowStatement::fromJson(const nlohmann::json& j) {
    auto stmt = std::make_unique<ThrowStatement>();
    stmt->location = getLocation(j);
    if (j.contains("expression")) {
        stmt->expression = Expression::fromJson(j["expression"]);
    }
    return stmt;
}

std::unique_ptr<SynchronizedStatement> SynchronizedStatement::fromJson(const nlohmann::json& j) {
    auto stmt = std::make_unique<SynchronizedStatement>();
    stmt->location = getLocation(j);
    if (j.contains("expression")) {
        stmt->expression = Expression::fromJson(j["expression"]);
    }
    if (j.contains("block")) {
        stmt->body = BlockStatement::fromJson(j["block"]);
    }
    return stmt;
}

std::unique_ptr<TryStatement> TryStatement::fromJson(const nlohmann::json& j) {
    auto stmt = std::make_unique<TryStatement>();
    stmt->location = getLocation(j);
    if (j.contains("block")) {
        stmt->tryBlock = BlockStatement::fromJson(j["block"]);
    }
    if (j.contains("catch_clauses") && j["catch_clauses"].is_array()) {
        for (const auto& c : j["catch_clauses"]) {
            TryStatement::CatchClause clause;
            if (c.contains("parameter")) {
                auto param = FormalParameter::fromJson(c["parameter"]);
                if (param) {
                    clause.modifiers = std::move(param->modifiers);
                    clause.type = std::move(param->type);
                    clause.variableName = param->name;
                }
            }
            if (c.contains("block")) {
                clause.block = BlockStatement::fromJson(c["block"]);
            }
            stmt->catchClauses.push_back(std::move(clause));
        }
    }
    if (j.contains("finally_clause") && !j["finally_clause"].is_null()) {
        if (j["finally_clause"].contains("block")) {
            stmt->finallyBlock = BlockStatement::fromJson(j["finally_clause"]["block"]);
        }
    }
    return stmt;
}

std::unique_ptr<AssertStatement> AssertStatement::fromJson(const nlohmann::json& j) {
    auto stmt = std::make_unique<AssertStatement>();
    stmt->location = getLocation(j);
    if (j.contains("condition")) {
        stmt->condition = Expression::fromJson(j["condition"]);
    }
    if (j.contains("message") && !j["message"].is_null()) {
        stmt->message = Expression::fromJson(j["message"]);
    }
    return stmt;
}

std::unique_ptr<ThisCall> ThisCall::fromJson(const nlohmann::json& j) {
    auto stmt = std::make_unique<ThisCall>();
    stmt->location = getLocation(j);
    if (j.contains("type_arguments") && j["type_arguments"].contains("arguments")) {
        for (const auto& arg : j["type_arguments"]["arguments"]) {
            stmt->typeArguments.push_back(Type::fromJson(arg));
        }
    }
    if (j.contains("arguments") && j["arguments"].contains("arguments")) {
        for (const auto& arg : j["arguments"]["arguments"]) {
            stmt->arguments.push_back(Expression::fromJson(arg));
        }
    }
    return stmt;
}

std::unique_ptr<SuperCall> SuperCall::fromJson(const nlohmann::json& j) {
    auto stmt = std::make_unique<SuperCall>();
    stmt->location = getLocation(j);
    if (j.contains("base") && !j["base"].is_null()) {
        stmt->base = Expression::fromJson(j["base"]);
    }
    if (j.contains("type_arguments") && j["type_arguments"].contains("arguments")) {
        for (const auto& arg : j["type_arguments"]["arguments"]) {
            stmt->typeArguments.push_back(Type::fromJson(arg));
        }
    }
    if (j.contains("arguments") && j["arguments"].contains("arguments")) {
        for (const auto& arg : j["arguments"]["arguments"]) {
            stmt->arguments.push_back(Expression::fromJson(arg));
        }
    }
    return stmt;
}

std::unique_ptr<LocalClassStatement> LocalClassStatement::fromJson(const nlohmann::json& j) {
    auto stmt = std::make_unique<LocalClassStatement>();
    stmt->location = getLocation(j);
    if (j.contains("declaration")) {
        stmt->declaration = ClassDeclaration::fromJson(j["declaration"]);
    }
    return stmt;
}

// Declaration implementations
std::unique_ptr<Declaration> Declaration::fromJson(const nlohmann::json& j) {
    if (j.is_null()) return nullptr;
    std::string kind = j.value("kind", "");

    if (kind == "FieldDeclaration") {
        return FieldDeclaration::fromJson(j);
    } else if (kind == "MethodDeclaration") {
        return MethodDeclaration::fromJson(j);
    } else if (kind == "ConstructorDeclaration") {
        return ConstructorDeclaration::fromJson(j);
    } else if (kind == "InitializerDeclaration") {
        return InitializerDeclaration::fromJson(j);
    } else if (kind == "ClassDeclaration") {
        return ClassDeclaration::fromJson(j);
    } else if (kind == "EnumDeclaration") {
        return EnumDeclaration::fromJson(j);
    } else if (kind == "InterfaceDeclaration") {
        return InterfaceDeclaration::fromJson(j);
    } else if (kind == "AnnotationDeclaration") {
        return AnnotationDeclaration::fromJson(j);
    } else if (kind == "EmptyDeclaration") {
        return EmptyDeclaration::fromJson(j);
    }
    return nullptr;
}

std::unique_ptr<FormalParameter> FormalParameter::fromJson(const nlohmann::json& j) {
    auto param = std::make_unique<FormalParameter>();
    param->location = getLocation(j);
    if (j.contains("modifiers") && !j["modifiers"].is_null()) {
        param->modifiers = Modifiers::fromJson(j["modifiers"]);
    }
    if (j.contains("type")) {
        param->type = Type::fromJson(j["type"]);
    }
    param->isVarargs = j.value("is_vararg", false);
    if (j.contains("declarator")) {
        auto decl = j["declarator"];
        if (decl.contains("name")) {
            if (decl["name"].is_string()) {
                param->name = decl["name"].get<std::string>();
            } else if (decl["name"].is_object() && decl["name"].contains("name")) {
                param->name = decl["name"]["name"].get<std::string>();
            }
        }
    }
    return param;
}

std::unique_ptr<MethodBody> MethodBody::fromJson(const nlohmann::json& j) {
    auto body = std::make_unique<MethodBody>();
    body->location = getLocation(j);
    if (j.contains("explicit_constructor") && !j["explicit_constructor"].is_null()) {
        body->explicitConstructorCall = Statement::fromJson(j["explicit_constructor"]);
    }
    if (j.contains("statements") && j["statements"].is_array()) {
        for (const auto& s : j["statements"]) {
            auto child = Statement::fromJson(s);
            if (child) body->statements.push_back(std::move(child));
        }
    }
    return body;
}

std::unique_ptr<FieldDeclaration> FieldDeclaration::fromJson(const nlohmann::json& j) {
    auto decl = std::make_unique<FieldDeclaration>();
    decl->location = getLocation(j);
    if (j.contains("modifiers") && !j["modifiers"].is_null()) {
        decl->modifiers = Modifiers::fromJson(j["modifiers"]);
    }
    if (j.contains("type")) {
        decl->type = Type::fromJson(j["type"]);
    }
    if (j.contains("declarators") && j["declarators"].is_array()) {
        for (const auto& d : j["declarators"]) {
            decl->declarators.push_back(VariableDeclarator::fromJson(d));
        }
    }
    return decl;
}

std::unique_ptr<MethodDeclaration> MethodDeclaration::fromJson(const nlohmann::json& j) {
    auto decl = std::make_unique<MethodDeclaration>();
    decl->location = getLocation(j);
    if (j.contains("modifiers") && !j["modifiers"].is_null()) {
        decl->modifiers = Modifiers::fromJson(j["modifiers"]);
    }
    if (j.contains("type_parameters") && j["type_parameters"].contains("parameters")) {
        for (const auto& p : j["type_parameters"]["parameters"]) {
            decl->typeParameters.push_back(TypeParameter::fromJson(p));
        }
    }
    if (j.contains("return_type")) {
        decl->returnType = Type::fromJson(j["return_type"]);
    }
    if (j.contains("declarator")) {
        auto d = j["declarator"];
        decl->name = d.value("name", "");
        if (d.contains("parameters") && d["parameters"].is_array()) {
            for (const auto& p : d["parameters"]) {
                decl->parameters.push_back(FormalParameter::fromJson(p));
            }
        }
    }
    if (j.contains("throws") && j["throws"].is_array()) {
        for (const auto& t : j["throws"]) {
            decl->throwsTypes.push_back(TypeName::fromJson(t));
        }
    }
    if (j.contains("body") && !j["body"].is_null()) {
        decl->body = MethodBody::fromJson(j["body"]);
    }
    if (j.contains("default_value") && !j["default_value"].is_null()) {
        decl->defaultValue = Expression::fromJson(j["default_value"]);
    }
    return decl;
}

std::unique_ptr<ConstructorDeclaration> ConstructorDeclaration::fromJson(const nlohmann::json& j) {
    auto decl = std::make_unique<ConstructorDeclaration>();
    decl->location = getLocation(j);
    if (j.contains("modifiers") && !j["modifiers"].is_null()) {
        decl->modifiers = Modifiers::fromJson(j["modifiers"]);
    }
    if (j.contains("type_parameters") && j["type_parameters"].contains("parameters")) {
        for (const auto& p : j["type_parameters"]["parameters"]) {
            decl->typeParameters.push_back(TypeParameter::fromJson(p));
        }
    }
    if (j.contains("declarator")) {
        auto d = j["declarator"];
        decl->name = d.value("name", "");
        if (d.contains("parameters") && d["parameters"].is_array()) {
            for (const auto& p : d["parameters"]) {
                decl->parameters.push_back(FormalParameter::fromJson(p));
            }
        }
    }
    if (j.contains("throws") && j["throws"].is_array()) {
        for (const auto& t : j["throws"]) {
            decl->throwsTypes.push_back(TypeName::fromJson(t));
        }
    }
    if (j.contains("body") && !j["body"].is_null()) {
        decl->body = MethodBody::fromJson(j["body"]);
    }
    return decl;
}

std::unique_ptr<InitializerDeclaration> InitializerDeclaration::fromJson(const nlohmann::json& j) {
    auto decl = std::make_unique<InitializerDeclaration>();
    decl->location = getLocation(j);
    if (j.contains("modifiers") && !j["modifiers"].is_null()) {
        decl->modifiers = Modifiers::fromJson(j["modifiers"]);
        decl->isStatic = decl->modifiers->hasModifier("static");
    }
    if (j.contains("body") && !j["body"].is_null()) {
        decl->body = MethodBody::fromJson(j["body"]);
    }
    return decl;
}

std::unique_ptr<ClassBody> ClassBody::fromJson(const nlohmann::json& j) {
    auto body = std::make_unique<ClassBody>();
    body->location = getLocation(j);
    if (j.contains("declarations") && j["declarations"].is_array()) {
        for (const auto& d : j["declarations"]) {
            auto child = Declaration::fromJson(d);
            if (child) body->declarations.push_back(std::move(child));
        }
    }
    return body;
}

std::unique_ptr<ClassDeclaration> ClassDeclaration::fromJson(const nlohmann::json& j) {
    auto decl = std::make_unique<ClassDeclaration>();
    decl->location = getLocation(j);
    if (j.contains("modifiers") && !j["modifiers"].is_null()) {
        decl->modifiers = Modifiers::fromJson(j["modifiers"]);
    }
    decl->name = j.value("name", "");
    if (j.contains("type_parameters") && j["type_parameters"].contains("parameters")) {
        for (const auto& p : j["type_parameters"]["parameters"]) {
            decl->typeParameters.push_back(TypeParameter::fromJson(p));
        }
    }
    if (j.contains("superclass") && !j["superclass"].is_null()) {
        decl->superclass = TypeName::fromJson(j["superclass"]);
    }
    if (j.contains("interfaces") && j["interfaces"].is_array()) {
        for (const auto& i : j["interfaces"]) {
            decl->interfaces.push_back(TypeName::fromJson(i));
        }
    }
    if (j.contains("body") && !j["body"].is_null()) {
        decl->body = ClassBody::fromJson(j["body"]);
    }
    return decl;
}

std::unique_ptr<EnumConstant> EnumConstant::fromJson(const nlohmann::json& j) {
    auto constant = std::make_unique<EnumConstant>();
    constant->location = getLocation(j);
    if (j.contains("modifiers") && !j["modifiers"].is_null()) {
        constant->modifiers = Modifiers::fromJson(j["modifiers"]);
    }
    constant->name = j.value("name", "");
    if (j.contains("arguments") && j["arguments"].contains("arguments")) {
        for (const auto& arg : j["arguments"]["arguments"]) {
            constant->arguments.push_back(Expression::fromJson(arg));
        }
    }
    if (j.contains("body") && !j["body"].is_null()) {
        constant->body = ClassBody::fromJson(j["body"]);
    }
    return constant;
}

std::unique_ptr<EnumDeclaration> EnumDeclaration::fromJson(const nlohmann::json& j) {
    auto decl = std::make_unique<EnumDeclaration>();
    decl->location = getLocation(j);
    if (j.contains("modifiers") && !j["modifiers"].is_null()) {
        decl->modifiers = Modifiers::fromJson(j["modifiers"]);
    }
    decl->name = j.value("name", "");
    if (j.contains("interfaces") && j["interfaces"].is_array()) {
        for (const auto& i : j["interfaces"]) {
            decl->interfaces.push_back(TypeName::fromJson(i));
        }
    }
    if (j.contains("constants") && j["constants"].is_array()) {
        for (const auto& c : j["constants"]) {
            decl->constants.push_back(EnumConstant::fromJson(c));
        }
    }
    if (j.contains("body") && !j["body"].is_null()) {
        decl->body = ClassBody::fromJson(j["body"]);
    }
    return decl;
}

std::unique_ptr<InterfaceDeclaration> InterfaceDeclaration::fromJson(const nlohmann::json& j) {
    auto decl = std::make_unique<InterfaceDeclaration>();
    decl->location = getLocation(j);
    if (j.contains("modifiers") && !j["modifiers"].is_null()) {
        decl->modifiers = Modifiers::fromJson(j["modifiers"]);
    }
    decl->name = j.value("name", "");
    if (j.contains("type_parameters") && j["type_parameters"].contains("parameters")) {
        for (const auto& p : j["type_parameters"]["parameters"]) {
            decl->typeParameters.push_back(TypeParameter::fromJson(p));
        }
    }
    if (j.contains("extends") && j["extends"].is_array()) {
        for (const auto& e : j["extends"]) {
            decl->extendsTypes.push_back(TypeName::fromJson(e));
        }
    }
    if (j.contains("body") && !j["body"].is_null()) {
        decl->body = ClassBody::fromJson(j["body"]);
    }
    return decl;
}

std::unique_ptr<AnnotationDeclaration> AnnotationDeclaration::fromJson(const nlohmann::json& j) {
    auto decl = std::make_unique<AnnotationDeclaration>();
    decl->location = getLocation(j);
    if (j.contains("modifiers") && !j["modifiers"].is_null()) {
        decl->modifiers = Modifiers::fromJson(j["modifiers"]);
    }
    decl->name = j.value("name", "");
    if (j.contains("body") && !j["body"].is_null()) {
        decl->body = ClassBody::fromJson(j["body"]);
    }
    return decl;
}

std::unique_ptr<EmptyDeclaration> EmptyDeclaration::fromJson(const nlohmann::json& j) {
    auto decl = std::make_unique<EmptyDeclaration>();
    decl->location = getLocation(j);
    return decl;
}

std::unique_ptr<PackageDeclaration> PackageDeclaration::fromJson(const nlohmann::json& j) {
    auto decl = std::make_unique<PackageDeclaration>();
    decl->location = getLocation(j);
    if (j.contains("modifiers") && !j["modifiers"].is_null()) {
        decl->modifiers = Modifiers::fromJson(j["modifiers"]);
    }
    if (j.contains("name")) {
        decl->name = Name::fromJson(j["name"]);
    }
    return decl;
}

std::unique_ptr<ImportDeclaration> ImportDeclaration::fromJson(const nlohmann::json& j) {
    auto decl = std::make_unique<ImportDeclaration>();
    decl->location = getLocation(j);
    if (j.contains("name")) {
        decl->name = Name::fromJson(j["name"]);
    }
    decl->isStatic = j.value("is_static", false);
    decl->isOnDemand = j.value("is_on_demand", false);
    return decl;
}

std::unique_ptr<CompilationUnit> CompilationUnit::fromJson(const nlohmann::json& j) {
    auto unit = std::make_unique<CompilationUnit>();
    unit->location = getLocation(j);
    if (j.contains("package") && !j["package"].is_null()) {
        unit->packageDecl = PackageDeclaration::fromJson(j["package"]);
    }
    if (j.contains("imports") && j["imports"].is_array()) {
        for (const auto& i : j["imports"]) {
            unit->imports.push_back(ImportDeclaration::fromJson(i));
        }
    }
    if (j.contains("types") && j["types"].is_array()) {
        for (const auto& t : j["types"]) {
            auto child = Declaration::fromJson(t);
            if (child) unit->types.push_back(std::move(child));
        }
    }
    return unit;
}

} // namespace jast
} // namespace Jopa
