#ifndef jast2_INCLUDED
#define jast2_INCLUDED

#include "platform.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <functional>

namespace Jopa {

// Forward declarations for symbols (reused from existing infrastructure)
class TypeSymbol;
class MethodSymbol;
class VariableSymbol;
class PackageSymbol;
class LabelSymbol;
class LexStream;

namespace jast2 {

// Forward declarations
class Node;
class Visitor;
class CompilationUnit;

// Location information tied to source tokens
struct SourceLocation {
    unsigned start_token = 0;  // Index into LexStream
    unsigned end_token = 0;
    int line = 0;
    int column = 0;

    static SourceLocation fromJson(const nlohmann::json& j);
};

//=============================================================================
// VISITOR PATTERN
//=============================================================================

// Forward declare all node types for visitor
class Name;
class Modifiers;
class Type;
class PrimitiveType;
class ArrayType;
class TypeName;
class WildcardType;
class TypeParameter;
class Expression;
class LiteralExpression;
class NameExpression;
class ThisExpression;
class SuperExpression;
class ParenthesizedExpression;
class BinaryExpression;
class UnaryExpression;
class AssignmentExpression;
class ConditionalExpression;
class InstanceofExpression;
class CastExpression;
class FieldAccessExpression;
class MethodInvocationExpression;
class ArrayAccessExpression;
class ClassCreationExpression;
class ArrayCreationExpression;
class ArrayInitializer;
class ClassLiteralExpression;
class Statement;
class BlockStatement;
class EmptyStatement;
class ExpressionStatement;
class LocalVariableStatement;
class IfStatement;
class WhileStatement;
class DoStatement;
class ForStatement;
class ForeachStatement;
class SwitchStatement;
class BreakStatement;
class ContinueStatement;
class ReturnStatement;
class ThrowStatement;
class SynchronizedStatement;
class TryStatement;
class AssertStatement;
class ThisCall;
class SuperCall;
class LocalClassStatement;
class LabeledStatement;
class Declaration;
class VariableDeclarator;
class FormalParameter;
class FieldDeclaration;
class MethodDeclaration;
class ConstructorDeclaration;
class InitializerDeclaration;
class ClassBody;
class ClassDeclaration;
class EnumConstant;
class EnumDeclaration;
class InterfaceDeclaration;
class AnnotationDeclaration;
class EmptyDeclaration;
class PackageDeclaration;
class ImportDeclaration;

// Visitor base class - implement to traverse JAST
class Visitor {
public:
    virtual ~Visitor() = default;

    // Types
    virtual void visit(Name*) {}
    virtual void visit(Modifiers*) {}
    virtual void visit(PrimitiveType*) {}
    virtual void visit(ArrayType*) {}
    virtual void visit(TypeName*) {}
    virtual void visit(WildcardType*) {}
    virtual void visit(TypeParameter*) {}

    // Expressions
    virtual void visit(LiteralExpression*) {}
    virtual void visit(NameExpression*) {}
    virtual void visit(ThisExpression*) {}
    virtual void visit(SuperExpression*) {}
    virtual void visit(ParenthesizedExpression*) {}
    virtual void visit(BinaryExpression*) {}
    virtual void visit(UnaryExpression*) {}
    virtual void visit(AssignmentExpression*) {}
    virtual void visit(ConditionalExpression*) {}
    virtual void visit(InstanceofExpression*) {}
    virtual void visit(CastExpression*) {}
    virtual void visit(FieldAccessExpression*) {}
    virtual void visit(MethodInvocationExpression*) {}
    virtual void visit(ArrayAccessExpression*) {}
    virtual void visit(ClassCreationExpression*) {}
    virtual void visit(ArrayCreationExpression*) {}
    virtual void visit(ArrayInitializer*) {}
    virtual void visit(ClassLiteralExpression*) {}

    // Statements
    virtual void visit(BlockStatement*) {}
    virtual void visit(EmptyStatement*) {}
    virtual void visit(ExpressionStatement*) {}
    virtual void visit(LocalVariableStatement*) {}
    virtual void visit(IfStatement*) {}
    virtual void visit(WhileStatement*) {}
    virtual void visit(DoStatement*) {}
    virtual void visit(ForStatement*) {}
    virtual void visit(ForeachStatement*) {}
    virtual void visit(SwitchStatement*) {}
    virtual void visit(BreakStatement*) {}
    virtual void visit(ContinueStatement*) {}
    virtual void visit(ReturnStatement*) {}
    virtual void visit(ThrowStatement*) {}
    virtual void visit(SynchronizedStatement*) {}
    virtual void visit(TryStatement*) {}
    virtual void visit(AssertStatement*) {}
    virtual void visit(ThisCall*) {}
    virtual void visit(SuperCall*) {}
    virtual void visit(LocalClassStatement*) {}
    virtual void visit(LabeledStatement*) {}

    // Declarations
    virtual void visit(VariableDeclarator*) {}
    virtual void visit(FormalParameter*) {}
    virtual void visit(FieldDeclaration*) {}
    virtual void visit(MethodDeclaration*) {}
    virtual void visit(ConstructorDeclaration*) {}
    virtual void visit(InitializerDeclaration*) {}
    virtual void visit(ClassBody*) {}
    virtual void visit(ClassDeclaration*) {}
    virtual void visit(EnumConstant*) {}
    virtual void visit(EnumDeclaration*) {}
    virtual void visit(InterfaceDeclaration*) {}
    virtual void visit(AnnotationDeclaration*) {}
    virtual void visit(EmptyDeclaration*) {}

    // Top-level
    virtual void visit(PackageDeclaration*) {}
    virtual void visit(ImportDeclaration*) {}
    virtual void visit(CompilationUnit*) {}
};

//=============================================================================
// BASE NODE CLASS
//=============================================================================

class Node {
public:
    virtual ~Node() = default;

    // Source location
    SourceLocation location;

    // Parent pointer for navigation (set during tree construction)
    Node* parent = nullptr;

    // Node kind for RTTI
    virtual std::string kind() const = 0;

    // Visitor pattern
    virtual void accept(Visitor* v) = 0;

    // Helper to walk children
    virtual void visitChildren(Visitor* /*v*/) {}

    // Construction from JSON
    static std::unique_ptr<Node> fromJson(const nlohmann::json& j, Node* parent = nullptr);

protected:
    // Set parent on a child node
    template<typename T>
    void setParent(std::unique_ptr<T>& child) {
        if (child) child->parent = this;
    }

    template<typename T>
    void setParent(std::vector<std::unique_ptr<T>>& children) {
        for (auto& child : children) {
            if (child) child->parent = this;
        }
    }
};

//=============================================================================
// NAME AND MODIFIERS
//=============================================================================

class Name : public Node {
public:
    std::string identifier;
    std::unique_ptr<Name> base;  // For qualified names

    // Semantic: resolved symbol (package, type, variable, etc.)
    union {
        PackageSymbol* package_symbol;
        TypeSymbol* type_symbol;
        VariableSymbol* variable_symbol;
    } resolved = {nullptr};
    enum class ResolvedKind { NONE, PACKAGE, TYPE, VARIABLE } resolved_kind = ResolvedKind::NONE;

    std::string kind() const override { return "Name"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (base) base->accept(v);
    }

    std::string getFullName() const;
    static std::unique_ptr<Name> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class Modifiers : public Node {
public:
    struct Modifier {
        std::string kind;   // "ModifierKeyword" or "Annotation"
        std::string value;  // "public", "static", etc. or annotation name
        // For annotations with values, we could add more fields
    };
    std::vector<Modifier> modifiers;

    std::string kind() const override { return "Modifiers"; }
    void accept(Visitor* v) override { v->visit(this); }

    bool hasModifier(const std::string& mod) const;
    bool isPublic() const { return hasModifier("public"); }
    bool isPrivate() const { return hasModifier("private"); }
    bool isProtected() const { return hasModifier("protected"); }
    bool isStatic() const { return hasModifier("static"); }
    bool isFinal() const { return hasModifier("final"); }
    bool isAbstract() const { return hasModifier("abstract"); }
    bool isNative() const { return hasModifier("native"); }
    bool isSynchronized() const { return hasModifier("synchronized"); }
    bool isTransient() const { return hasModifier("transient"); }
    bool isVolatile() const { return hasModifier("volatile"); }
    bool isStrictfp() const { return hasModifier("strictfp"); }

    static std::unique_ptr<Modifiers> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

//=============================================================================
// TYPES
//=============================================================================

class Type : public Node {
public:
    // Semantic: resolved type
    TypeSymbol* resolved_type = nullptr;

    static std::unique_ptr<Type> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class PrimitiveType : public Type {
public:
    std::string type;  // "int", "boolean", "void", etc.

    std::string kind() const override { return "PrimitiveType"; }
    void accept(Visitor* v) override { v->visit(this); }
    static std::unique_ptr<PrimitiveType> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class ArrayType : public Type {
public:
    std::unique_ptr<Type> elementType;
    int dimensions = 1;

    std::string kind() const override { return "ArrayType"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (elementType) elementType->accept(v);
    }
    static std::unique_ptr<ArrayType> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class TypeName : public Type {
public:
    std::unique_ptr<Name> name;
    std::unique_ptr<TypeName> base;  // For qualified types like Outer.Inner
    std::vector<std::unique_ptr<Type>> typeArguments;

    std::string kind() const override { return "TypeName"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (name) name->accept(v);
        if (base) base->accept(v);
        for (auto& arg : typeArguments) if (arg) arg->accept(v);
    }
    static std::unique_ptr<TypeName> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class WildcardType : public Type {
public:
    enum class BoundKind { NONE, EXTENDS, SUPER };
    BoundKind boundKind = BoundKind::NONE;
    std::unique_ptr<Type> bound;

    std::string kind() const override { return "Wildcard"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (bound) bound->accept(v);
    }
    static std::unique_ptr<WildcardType> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class TypeParameter : public Node {
public:
    std::string name;
    std::vector<std::unique_ptr<TypeName>> bounds;

    // Semantic: type variable symbol
    TypeSymbol* type_variable = nullptr;

    std::string kind() const override { return "TypeParameter"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        for (auto& bound : bounds) if (bound) bound->accept(v);
    }
    static std::unique_ptr<TypeParameter> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

//=============================================================================
// EXPRESSIONS
//=============================================================================

class Expression : public Node {
public:
    // Semantic: resolved type of this expression
    TypeSymbol* resolved_type = nullptr;

    // Is this expression a constant (compile-time evaluable)?
    bool is_constant = false;

    // For constant expressions, the value (stored as variant or separate fields)
    // Using a simple representation for now
    union ConstantValue {
        long long int_value;
        double float_value;
        bool bool_value;
    } constant_value = {0};
    enum class ConstantKind { NONE, INT, LONG, FLOAT, DOUBLE, BOOLEAN, STRING, NULL_VAL } constant_kind = ConstantKind::NONE;
    std::string string_constant;  // For string constants

    static std::unique_ptr<Expression> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class LiteralExpression : public Expression {
public:
    std::string literalKind;  // "Integer", "Long", "Float", "Double", "String", "Character", "Boolean", "Null"
    std::string value;

    std::string kind() const override { return literalKind + "Literal"; }
    void accept(Visitor* v) override { v->visit(this); }
    static std::unique_ptr<LiteralExpression> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class NameExpression : public Expression {
public:
    std::unique_ptr<Name> name;

    // Semantic: what this name resolved to
    VariableSymbol* resolved_variable = nullptr;
    TypeSymbol* resolved_type_name = nullptr;  // For type names used as expressions
    PackageSymbol* resolved_package = nullptr;

    std::string kind() const override { return "NameExpression"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (name) name->accept(v);
    }
    static std::unique_ptr<NameExpression> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class ThisExpression : public Expression {
public:
    std::unique_ptr<TypeName> qualifier;

    std::string kind() const override { return "ThisExpression"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (qualifier) qualifier->accept(v);
    }
    static std::unique_ptr<ThisExpression> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class SuperExpression : public Expression {
public:
    std::unique_ptr<TypeName> qualifier;

    std::string kind() const override { return "SuperExpression"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (qualifier) qualifier->accept(v);
    }
    static std::unique_ptr<SuperExpression> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class ParenthesizedExpression : public Expression {
public:
    std::unique_ptr<Expression> expression;

    std::string kind() const override { return "ParenthesizedExpression"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (expression) expression->accept(v);
    }
    static std::unique_ptr<ParenthesizedExpression> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class BinaryExpression : public Expression {
public:
    std::string op;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;

    std::string kind() const override { return "BinaryExpression"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (left) left->accept(v);
        if (right) right->accept(v);
    }
    static std::unique_ptr<BinaryExpression> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class UnaryExpression : public Expression {
public:
    std::string op;
    bool isPrefix;
    std::unique_ptr<Expression> operand;

    std::string kind() const override { return isPrefix ? "PreUnaryExpression" : "PostUnaryExpression"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (operand) operand->accept(v);
    }
    static std::unique_ptr<UnaryExpression> fromJson(const nlohmann::json& j, Node* parent, bool prefix);
};

class AssignmentExpression : public Expression {
public:
    std::string op;  // "=", "+=", etc.
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;

    std::string kind() const override { return "AssignmentExpression"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (left) left->accept(v);
        if (right) right->accept(v);
    }
    static std::unique_ptr<AssignmentExpression> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class ConditionalExpression : public Expression {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Expression> thenExpr;
    std::unique_ptr<Expression> elseExpr;

    std::string kind() const override { return "ConditionalExpression"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (condition) condition->accept(v);
        if (thenExpr) thenExpr->accept(v);
        if (elseExpr) elseExpr->accept(v);
    }
    static std::unique_ptr<ConditionalExpression> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class InstanceofExpression : public Expression {
public:
    std::unique_ptr<Expression> expression;
    std::unique_ptr<Type> type;

    std::string kind() const override { return "InstanceofExpression"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (expression) expression->accept(v);
        if (type) type->accept(v);
    }
    static std::unique_ptr<InstanceofExpression> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class CastExpression : public Expression {
public:
    std::unique_ptr<Type> type;
    std::unique_ptr<Expression> expression;

    std::string kind() const override { return "CastExpression"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (type) type->accept(v);
        if (expression) expression->accept(v);
    }
    static std::unique_ptr<CastExpression> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class FieldAccessExpression : public Expression {
public:
    std::unique_ptr<Expression> base;
    std::string identifier;

    // Semantic: resolved field
    VariableSymbol* resolved_field = nullptr;

    std::string kind() const override { return "FieldAccess"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (base) base->accept(v);
    }
    static std::unique_ptr<FieldAccessExpression> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class MethodInvocationExpression : public Expression {
public:
    std::unique_ptr<Expression> base;
    std::string methodName;
    std::vector<std::unique_ptr<Type>> typeArguments;
    std::vector<std::unique_ptr<Expression>> arguments;

    // Semantic: resolved method
    MethodSymbol* resolved_method = nullptr;

    std::string kind() const override { return "MethodInvocation"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (base) base->accept(v);
        for (auto& arg : typeArguments) if (arg) arg->accept(v);
        for (auto& arg : arguments) if (arg) arg->accept(v);
    }
    static std::unique_ptr<MethodInvocationExpression> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class ArrayAccessExpression : public Expression {
public:
    std::unique_ptr<Expression> array;
    std::unique_ptr<Expression> index;

    std::string kind() const override { return "ArrayAccess"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (array) array->accept(v);
        if (index) index->accept(v);
    }
    static std::unique_ptr<ArrayAccessExpression> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class ClassCreationExpression : public Expression {
public:
    std::unique_ptr<Expression> base;  // For inner class creation
    std::unique_ptr<TypeName> type;
    std::vector<std::unique_ptr<Type>> typeArguments;
    std::vector<std::unique_ptr<Expression>> arguments;
    std::unique_ptr<ClassBody> classBody;  // For anonymous classes

    // Semantic: resolved constructor
    MethodSymbol* resolved_constructor = nullptr;
    // For anonymous classes, the generated type
    TypeSymbol* anonymous_type = nullptr;

    std::string kind() const override { return "ClassCreationExpression"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override;
    static std::unique_ptr<ClassCreationExpression> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class ArrayCreationExpression : public Expression {
public:
    std::unique_ptr<Type> elementType;
    std::vector<std::unique_ptr<Expression>> dimensionExprs;
    int extraDims = 0;
    std::unique_ptr<ArrayInitializer> initializer;

    std::string kind() const override { return "ArrayCreationExpression"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override;
    static std::unique_ptr<ArrayCreationExpression> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class ArrayInitializer : public Expression {
public:
    std::vector<std::unique_ptr<Expression>> elements;

    std::string kind() const override { return "ArrayInitializer"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        for (auto& elem : elements) if (elem) elem->accept(v);
    }
    static std::unique_ptr<ArrayInitializer> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class ClassLiteralExpression : public Expression {
public:
    std::unique_ptr<Type> type;

    std::string kind() const override { return "ClassLiteral"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (type) type->accept(v);
    }
    static std::unique_ptr<ClassLiteralExpression> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

//=============================================================================
// STATEMENTS
//=============================================================================

class Statement : public Node {
public:
    // Semantic: is this statement reachable?
    bool is_reachable = true;
    // Does this statement complete normally?
    bool can_complete_normally = true;

    static std::unique_ptr<Statement> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class BlockStatement : public Statement {
public:
    std::vector<std::unique_ptr<Statement>> statements;

    std::string kind() const override { return "Block"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        for (auto& stmt : statements) if (stmt) stmt->accept(v);
    }
    static std::unique_ptr<BlockStatement> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class EmptyStatement : public Statement {
public:
    std::string kind() const override { return "EmptyStatement"; }
    void accept(Visitor* v) override { v->visit(this); }
    static std::unique_ptr<EmptyStatement> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class ExpressionStatement : public Statement {
public:
    std::unique_ptr<Expression> expression;

    std::string kind() const override { return "ExpressionStatement"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (expression) expression->accept(v);
    }
    static std::unique_ptr<ExpressionStatement> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class VariableDeclarator : public Node {
public:
    std::string name;
    int extraDims = 0;
    std::unique_ptr<Expression> initializer;

    // Semantic: declared variable symbol
    VariableSymbol* symbol = nullptr;

    std::string kind() const override { return "VariableDeclarator"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (initializer) initializer->accept(v);
    }
    static std::unique_ptr<VariableDeclarator> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class LocalVariableStatement : public Statement {
public:
    std::unique_ptr<Modifiers> modifiers;
    std::unique_ptr<Type> type;
    std::vector<std::unique_ptr<VariableDeclarator>> declarators;

    std::string kind() const override { return "LocalVariableStatement"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (modifiers) modifiers->accept(v);
        if (type) type->accept(v);
        for (auto& decl : declarators) if (decl) decl->accept(v);
    }
    static std::unique_ptr<LocalVariableStatement> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class IfStatement : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> thenStatement;
    std::unique_ptr<Statement> elseStatement;

    std::string kind() const override { return "IfStatement"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (condition) condition->accept(v);
        if (thenStatement) thenStatement->accept(v);
        if (elseStatement) elseStatement->accept(v);
    }
    static std::unique_ptr<IfStatement> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class WhileStatement : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> body;

    std::string kind() const override { return "WhileStatement"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (condition) condition->accept(v);
        if (body) body->accept(v);
    }
    static std::unique_ptr<WhileStatement> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class DoStatement : public Statement {
public:
    std::unique_ptr<Statement> body;
    std::unique_ptr<Expression> condition;

    std::string kind() const override { return "DoStatement"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (body) body->accept(v);
        if (condition) condition->accept(v);
    }
    static std::unique_ptr<DoStatement> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class ForStatement : public Statement {
public:
    std::vector<std::unique_ptr<Statement>> init;
    std::unique_ptr<Expression> condition;
    std::vector<std::unique_ptr<Expression>> update;
    std::unique_ptr<Statement> body;

    std::string kind() const override { return "ForStatement"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        for (auto& stmt : init) if (stmt) stmt->accept(v);
        if (condition) condition->accept(v);
        for (auto& expr : update) if (expr) expr->accept(v);
        if (body) body->accept(v);
    }
    static std::unique_ptr<ForStatement> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class ForeachStatement : public Statement {
public:
    std::unique_ptr<Modifiers> modifiers;
    std::unique_ptr<Type> type;
    std::string variableName;
    std::unique_ptr<Expression> iterable;
    std::unique_ptr<Statement> body;

    // Semantic: loop variable
    VariableSymbol* variable_symbol = nullptr;

    std::string kind() const override { return "ForeachStatement"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (modifiers) modifiers->accept(v);
        if (type) type->accept(v);
        if (iterable) iterable->accept(v);
        if (body) body->accept(v);
    }
    static std::unique_ptr<ForeachStatement> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class SwitchStatement : public Statement {
public:
    struct SwitchLabel {
        bool isDefault = false;
        std::unique_ptr<Expression> expression;
    };
    struct SwitchBlock {
        std::vector<SwitchLabel> labels;
        std::vector<std::unique_ptr<Statement>> statements;
    };

    std::unique_ptr<Expression> expression;
    std::vector<SwitchBlock> blocks;

    std::string kind() const override { return "SwitchStatement"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override;
    static std::unique_ptr<SwitchStatement> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class BreakStatement : public Statement {
public:
    std::optional<std::string> label;

    // Semantic: target statement
    Statement* target = nullptr;

    std::string kind() const override { return "BreakStatement"; }
    void accept(Visitor* v) override { v->visit(this); }
    static std::unique_ptr<BreakStatement> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class ContinueStatement : public Statement {
public:
    std::optional<std::string> label;

    // Semantic: target statement
    Statement* target = nullptr;

    std::string kind() const override { return "ContinueStatement"; }
    void accept(Visitor* v) override { v->visit(this); }
    static std::unique_ptr<ContinueStatement> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class ReturnStatement : public Statement {
public:
    std::unique_ptr<Expression> expression;

    std::string kind() const override { return "ReturnStatement"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (expression) expression->accept(v);
    }
    static std::unique_ptr<ReturnStatement> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class ThrowStatement : public Statement {
public:
    std::unique_ptr<Expression> expression;

    std::string kind() const override { return "ThrowStatement"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (expression) expression->accept(v);
    }
    static std::unique_ptr<ThrowStatement> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class SynchronizedStatement : public Statement {
public:
    std::unique_ptr<Expression> expression;
    std::unique_ptr<BlockStatement> body;

    std::string kind() const override { return "SynchronizedStatement"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (expression) expression->accept(v);
        if (body) body->accept(v);
    }
    static std::unique_ptr<SynchronizedStatement> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class TryStatement : public Statement {
public:
    struct CatchClause {
        std::unique_ptr<Modifiers> modifiers;
        std::vector<std::unique_ptr<Type>> types;  // Multi-catch in Java 7+
        std::string variableName;
        std::unique_ptr<BlockStatement> block;
        VariableSymbol* variable_symbol = nullptr;  // Semantic
    };

    std::unique_ptr<BlockStatement> tryBlock;
    std::vector<CatchClause> catchClauses;
    std::unique_ptr<BlockStatement> finallyBlock;

    std::string kind() const override { return "TryStatement"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override;
    static std::unique_ptr<TryStatement> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class AssertStatement : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Expression> message;

    std::string kind() const override { return "AssertStatement"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (condition) condition->accept(v);
        if (message) message->accept(v);
    }
    static std::unique_ptr<AssertStatement> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class ThisCall : public Statement {
public:
    std::vector<std::unique_ptr<Type>> typeArguments;
    std::vector<std::unique_ptr<Expression>> arguments;

    // Semantic: resolved constructor
    MethodSymbol* resolved_constructor = nullptr;

    std::string kind() const override { return "ThisCall"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        for (auto& arg : typeArguments) if (arg) arg->accept(v);
        for (auto& arg : arguments) if (arg) arg->accept(v);
    }
    static std::unique_ptr<ThisCall> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class SuperCall : public Statement {
public:
    std::unique_ptr<Expression> base;
    std::vector<std::unique_ptr<Type>> typeArguments;
    std::vector<std::unique_ptr<Expression>> arguments;

    // Semantic: resolved constructor
    MethodSymbol* resolved_constructor = nullptr;

    std::string kind() const override { return "SuperCall"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (base) base->accept(v);
        for (auto& arg : typeArguments) if (arg) arg->accept(v);
        for (auto& arg : arguments) if (arg) arg->accept(v);
    }
    static std::unique_ptr<SuperCall> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class LocalClassStatement : public Statement {
public:
    std::unique_ptr<ClassDeclaration> declaration;

    std::string kind() const override { return "LocalClassStatement"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override;
    static std::unique_ptr<LocalClassStatement> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class LabeledStatement : public Statement {
public:
    std::string label;
    std::unique_ptr<Statement> statement;

    // Semantic: label symbol
    LabelSymbol* label_symbol = nullptr;

    std::string kind() const override { return "LabeledStatement"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (statement) statement->accept(v);
    }
    static std::unique_ptr<LabeledStatement> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

//=============================================================================
// DECLARATIONS
//=============================================================================

class Declaration : public Node {
public:
    std::unique_ptr<Modifiers> modifiers;

    static std::unique_ptr<Declaration> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class FormalParameter : public Node {
public:
    std::unique_ptr<Modifiers> modifiers;
    std::unique_ptr<Type> type;
    std::string name;
    int extraDims = 0;
    bool isVarargs = false;

    // Semantic: parameter symbol
    VariableSymbol* symbol = nullptr;

    std::string kind() const override { return "FormalParameter"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (modifiers) modifiers->accept(v);
        if (type) type->accept(v);
    }
    static std::unique_ptr<FormalParameter> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class MethodBody : public Node {
public:
    std::unique_ptr<Statement> explicitConstructorCall;  // this() or super()
    std::vector<std::unique_ptr<Statement>> statements;

    std::string kind() const override { return "MethodBody"; }
    void accept(Visitor* /*v*/) override {}  // Visited through parent
    void visitChildren(Visitor* v) override {
        if (explicitConstructorCall) explicitConstructorCall->accept(v);
        for (auto& stmt : statements) if (stmt) stmt->accept(v);
    }
    static std::unique_ptr<MethodBody> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class FieldDeclaration : public Declaration {
public:
    std::unique_ptr<Type> type;
    std::vector<std::unique_ptr<VariableDeclarator>> declarators;

    std::string kind() const override { return "FieldDeclaration"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (modifiers) modifiers->accept(v);
        if (type) type->accept(v);
        for (auto& decl : declarators) if (decl) decl->accept(v);
    }
    static std::unique_ptr<FieldDeclaration> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class MethodDeclaration : public Declaration {
public:
    std::vector<std::unique_ptr<TypeParameter>> typeParameters;
    std::unique_ptr<Type> returnType;
    std::string name;
    std::vector<std::unique_ptr<FormalParameter>> parameters;
    int extraDims = 0;
    std::vector<std::unique_ptr<TypeName>> throwsTypes;
    std::unique_ptr<MethodBody> body;
    std::unique_ptr<Expression> defaultValue;  // For annotation methods

    // Semantic: method symbol
    MethodSymbol* symbol = nullptr;

    std::string kind() const override { return "MethodDeclaration"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (modifiers) modifiers->accept(v);
        for (auto& tp : typeParameters) if (tp) tp->accept(v);
        if (returnType) returnType->accept(v);
        for (auto& param : parameters) if (param) param->accept(v);
        for (auto& t : throwsTypes) if (t) t->accept(v);
        if (body) body->visitChildren(v);
        if (defaultValue) defaultValue->accept(v);
    }
    static std::unique_ptr<MethodDeclaration> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class ConstructorDeclaration : public Declaration {
public:
    std::vector<std::unique_ptr<TypeParameter>> typeParameters;
    std::string name;
    std::vector<std::unique_ptr<FormalParameter>> parameters;
    std::vector<std::unique_ptr<TypeName>> throwsTypes;
    std::unique_ptr<MethodBody> body;

    // Semantic: constructor symbol
    MethodSymbol* symbol = nullptr;

    std::string kind() const override { return "ConstructorDeclaration"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (modifiers) modifiers->accept(v);
        for (auto& tp : typeParameters) if (tp) tp->accept(v);
        for (auto& param : parameters) if (param) param->accept(v);
        for (auto& t : throwsTypes) if (t) t->accept(v);
        if (body) body->visitChildren(v);
    }
    static std::unique_ptr<ConstructorDeclaration> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class InitializerDeclaration : public Declaration {
public:
    bool isStatic = false;
    std::unique_ptr<MethodBody> body;

    std::string kind() const override { return "InitializerDeclaration"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (body) body->visitChildren(v);
    }
    static std::unique_ptr<InitializerDeclaration> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class ClassBody : public Node {
public:
    std::vector<std::unique_ptr<Declaration>> declarations;

    std::string kind() const override { return "ClassBody"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        for (auto& decl : declarations) if (decl) decl->accept(v);
    }
    static std::unique_ptr<ClassBody> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class ClassDeclaration : public Declaration {
public:
    std::string name;
    std::vector<std::unique_ptr<TypeParameter>> typeParameters;
    std::unique_ptr<TypeName> superclass;
    std::vector<std::unique_ptr<TypeName>> interfaces;
    std::unique_ptr<ClassBody> body;

    // Semantic: type symbol
    TypeSymbol* symbol = nullptr;

    std::string kind() const override { return "ClassDeclaration"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (modifiers) modifiers->accept(v);
        for (auto& tp : typeParameters) if (tp) tp->accept(v);
        if (superclass) superclass->accept(v);
        for (auto& iface : interfaces) if (iface) iface->accept(v);
        if (body) body->accept(v);
    }
    static std::unique_ptr<ClassDeclaration> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class EnumConstant : public Declaration {
public:
    std::string name;
    std::vector<std::unique_ptr<Expression>> arguments;
    std::unique_ptr<ClassBody> body;

    // Semantic: variable symbol for the constant
    VariableSymbol* symbol = nullptr;

    std::string kind() const override { return "EnumConstant"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (modifiers) modifiers->accept(v);
        for (auto& arg : arguments) if (arg) arg->accept(v);
        if (body) body->accept(v);
    }
    static std::unique_ptr<EnumConstant> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class EnumDeclaration : public Declaration {
public:
    std::string name;
    std::vector<std::unique_ptr<TypeName>> interfaces;
    std::vector<std::unique_ptr<EnumConstant>> constants;
    std::unique_ptr<ClassBody> body;

    // Semantic: type symbol
    TypeSymbol* symbol = nullptr;

    std::string kind() const override { return "EnumDeclaration"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (modifiers) modifiers->accept(v);
        for (auto& iface : interfaces) if (iface) iface->accept(v);
        for (auto& constant : constants) if (constant) constant->accept(v);
        if (body) body->accept(v);
    }
    static std::unique_ptr<EnumDeclaration> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class InterfaceDeclaration : public Declaration {
public:
    std::string name;
    std::vector<std::unique_ptr<TypeParameter>> typeParameters;
    std::vector<std::unique_ptr<TypeName>> extendsTypes;
    std::unique_ptr<ClassBody> body;

    // Semantic: type symbol
    TypeSymbol* symbol = nullptr;

    std::string kind() const override { return "InterfaceDeclaration"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (modifiers) modifiers->accept(v);
        for (auto& tp : typeParameters) if (tp) tp->accept(v);
        for (auto& ext : extendsTypes) if (ext) ext->accept(v);
        if (body) body->accept(v);
    }
    static std::unique_ptr<InterfaceDeclaration> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class AnnotationDeclaration : public Declaration {
public:
    std::string name;
    std::unique_ptr<ClassBody> body;

    // Semantic: type symbol
    TypeSymbol* symbol = nullptr;

    std::string kind() const override { return "AnnotationDeclaration"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (modifiers) modifiers->accept(v);
        if (body) body->accept(v);
    }
    static std::unique_ptr<AnnotationDeclaration> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class EmptyDeclaration : public Declaration {
public:
    std::string kind() const override { return "EmptyDeclaration"; }
    void accept(Visitor* v) override { v->visit(this); }
    static std::unique_ptr<EmptyDeclaration> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

//=============================================================================
// TOP-LEVEL
//=============================================================================

class PackageDeclaration : public Node {
public:
    std::unique_ptr<Modifiers> modifiers;  // Annotations
    std::unique_ptr<Name> name;

    // Semantic: resolved package
    PackageSymbol* symbol = nullptr;

    std::string kind() const override { return "PackageDeclaration"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (modifiers) modifiers->accept(v);
        if (name) name->accept(v);
    }
    static std::unique_ptr<PackageDeclaration> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class ImportDeclaration : public Node {
public:
    std::unique_ptr<Name> name;
    bool isStatic = false;
    bool isOnDemand = false;

    // Semantic: what was imported
    TypeSymbol* imported_type = nullptr;
    PackageSymbol* imported_package = nullptr;

    std::string kind() const override { return "ImportDeclaration"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (name) name->accept(v);
    }
    static std::unique_ptr<ImportDeclaration> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

class CompilationUnit : public Node {
public:
    std::unique_ptr<PackageDeclaration> packageDecl;
    std::vector<std::unique_ptr<ImportDeclaration>> imports;
    std::vector<std::unique_ptr<Declaration>> types;

    // Source file reference
    LexStream* lex_stream = nullptr;

    // Semantic: associated package
    PackageSymbol* package_symbol = nullptr;

    // Did semantic analysis find errors?
    bool has_errors = false;

    std::string kind() const override { return "CompilationUnit"; }
    void accept(Visitor* v) override { v->visit(this); }
    void visitChildren(Visitor* v) override {
        if (packageDecl) packageDecl->accept(v);
        for (auto& import : imports) if (import) import->accept(v);
        for (auto& type : types) if (type) type->accept(v);
    }
    static std::unique_ptr<CompilationUnit> fromJson(const nlohmann::json& j, Node* parent = nullptr);
};

} // namespace jast2
} // namespace Jopa

#endif // jast2_INCLUDED
