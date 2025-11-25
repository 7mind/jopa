#ifndef jast_INCLUDED
#define jast_INCLUDED

#include "platform.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace Jopa {
namespace jast {

// Location information for source mapping
struct Location {
    int line = 0;
    int column = 0;

    static Location fromJson(const nlohmann::json& j);
    nlohmann::json toJson() const;
};

// Forward declarations
class Node;
class Name;
class Type;
class Expression;
class Statement;
class Declaration;
class CompilationUnit;

// Base class for all JAST nodes
class Node {
public:
    virtual ~Node() = default;
    Location location;

    virtual std::string kind() const = 0;
    static std::unique_ptr<Node> fromJson(const nlohmann::json& j);
};

// Represents a simple or qualified name (e.g., "foo" or "java.lang.String")
class Name : public Node {
public:
    std::string identifier;
    std::unique_ptr<Name> base;

    std::string kind() const override { return "Name"; }
    std::string getFullName() const;
    static std::unique_ptr<Name> fromJson(const nlohmann::json& j);
};

// Represents modifiers (public, static, etc.) including annotations
class Modifiers : public Node {
public:
    struct Modifier {
        std::string kind; // "keyword" or "annotation"
        std::string value; // modifier keyword or annotation name
    };
    std::vector<Modifier> modifiers;

    std::string kind() const override { return "Modifiers"; }
    bool hasModifier(const std::string& mod) const;
    static std::unique_ptr<Modifiers> fromJson(const nlohmann::json& j);
};

// Base class for type representations
class Type : public Node {
public:
    static std::unique_ptr<Type> fromJson(const nlohmann::json& j);
};

// Primitive type (int, boolean, etc.)
class PrimitiveType : public Type {
public:
    std::string type; // "int", "boolean", etc.

    std::string kind() const override { return "PrimitiveType"; }
    static std::unique_ptr<PrimitiveType> fromJson(const nlohmann::json& j);
};

// Array type (e.g., int[], String[][])
class ArrayType : public Type {
public:
    std::unique_ptr<Type> elementType;
    int dimensions = 1;

    std::string kind() const override { return "ArrayType"; }
    static std::unique_ptr<ArrayType> fromJson(const nlohmann::json& j);
};

// Reference type (e.g., String, java.util.List<T>)
class TypeName : public Type {
public:
    std::unique_ptr<Name> name;
    std::unique_ptr<TypeName> base; // For qualified types like Outer.Inner
    std::vector<std::unique_ptr<Type>> typeArguments;

    std::string kind() const override { return "TypeName"; }
    static std::unique_ptr<TypeName> fromJson(const nlohmann::json& j);
};

// Wildcard type (?)
class WildcardType : public Type {
public:
    enum class BoundKind { NONE, EXTENDS, SUPER };
    BoundKind boundKind = BoundKind::NONE;
    std::unique_ptr<Type> bound;

    std::string kind() const override { return "Wildcard"; }
    static std::unique_ptr<WildcardType> fromJson(const nlohmann::json& j);
};

// Type parameter (T extends Comparable<T>)
class TypeParameter : public Node {
public:
    std::string name;
    std::vector<std::unique_ptr<TypeName>> bounds;

    std::string kind() const override { return "TypeParameter"; }
    static std::unique_ptr<TypeParameter> fromJson(const nlohmann::json& j);
};

// Base class for expressions
class Expression : public Node {
public:
    static std::unique_ptr<Expression> fromJson(const nlohmann::json& j);
};

// Literal expressions (42, "hello", true, null, etc.)
class LiteralExpression : public Expression {
public:
    std::string literalKind; // "Integer", "String", "Boolean", "Null", etc.
    std::string value;

    std::string kind() const override { return literalKind + "Literal"; }
    static std::unique_ptr<LiteralExpression> fromJson(const nlohmann::json& j);
};

// Name expression (simple identifier or qualified name)
class NameExpression : public Expression {
public:
    std::unique_ptr<Name> name;

    std::string kind() const override { return "NameExpression"; }
    static std::unique_ptr<NameExpression> fromJson(const nlohmann::json& j);
};

// this expression
class ThisExpression : public Expression {
public:
    std::unique_ptr<TypeName> qualifier; // For qualified this (Outer.this)

    std::string kind() const override { return "ThisExpression"; }
    static std::unique_ptr<ThisExpression> fromJson(const nlohmann::json& j);
};

// super expression
class SuperExpression : public Expression {
public:
    std::unique_ptr<TypeName> qualifier; // For qualified super

    std::string kind() const override { return "SuperExpression"; }
    static std::unique_ptr<SuperExpression> fromJson(const nlohmann::json& j);
};

// Parenthesized expression ((expr))
class ParenthesizedExpression : public Expression {
public:
    std::unique_ptr<Expression> expression;

    std::string kind() const override { return "ParenthesizedExpression"; }
    static std::unique_ptr<ParenthesizedExpression> fromJson(const nlohmann::json& j);
};

// Binary expression (a + b, a && b, etc.)
class BinaryExpression : public Expression {
public:
    std::string op; // "+", "-", "&&", "||", etc.
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;

    std::string kind() const override { return "BinaryExpression"; }
    static std::unique_ptr<BinaryExpression> fromJson(const nlohmann::json& j);
};

// Unary expression (++a, -b, !c, etc.)
class UnaryExpression : public Expression {
public:
    std::string op;
    bool isPrefix;
    std::unique_ptr<Expression> operand;

    std::string kind() const override { return isPrefix ? "PreUnaryExpression" : "PostUnaryExpression"; }
    static std::unique_ptr<UnaryExpression> fromJson(const nlohmann::json& j, bool isPrefix);
};

// Assignment expression (a = b, a += b, etc.)
class AssignmentExpression : public Expression {
public:
    std::string op; // "=", "+=", etc.
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;

    std::string kind() const override { return "AssignmentExpression"; }
    static std::unique_ptr<AssignmentExpression> fromJson(const nlohmann::json& j);
};

// Conditional expression (a ? b : c)
class ConditionalExpression : public Expression {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Expression> thenExpr;
    std::unique_ptr<Expression> elseExpr;

    std::string kind() const override { return "ConditionalExpression"; }
    static std::unique_ptr<ConditionalExpression> fromJson(const nlohmann::json& j);
};

// instanceof expression
class InstanceofExpression : public Expression {
public:
    std::unique_ptr<Expression> expression;
    std::unique_ptr<Type> type;

    std::string kind() const override { return "InstanceofExpression"; }
    static std::unique_ptr<InstanceofExpression> fromJson(const nlohmann::json& j);
};

// Cast expression ((Type) expr)
class CastExpression : public Expression {
public:
    std::unique_ptr<Type> type;
    std::unique_ptr<Expression> expression;

    std::string kind() const override { return "CastExpression"; }
    static std::unique_ptr<CastExpression> fromJson(const nlohmann::json& j);
};

// Field access (obj.field)
class FieldAccessExpression : public Expression {
public:
    std::unique_ptr<Expression> base;
    std::string identifier;

    std::string kind() const override { return "FieldAccess"; }
    static std::unique_ptr<FieldAccessExpression> fromJson(const nlohmann::json& j);
};

// Method invocation (obj.method(args))
class MethodInvocationExpression : public Expression {
public:
    std::unique_ptr<Expression> base; // null for simple call
    std::string methodName;
    std::vector<std::unique_ptr<Type>> typeArguments;
    std::vector<std::unique_ptr<Expression>> arguments;

    std::string kind() const override { return "MethodInvocation"; }
    static std::unique_ptr<MethodInvocationExpression> fromJson(const nlohmann::json& j);
};

// Array access (arr[index])
class ArrayAccessExpression : public Expression {
public:
    std::unique_ptr<Expression> array;
    std::unique_ptr<Expression> index;

    std::string kind() const override { return "ArrayAccess"; }
    static std::unique_ptr<ArrayAccessExpression> fromJson(const nlohmann::json& j);
};

// new ClassName(args)
class ClassCreationExpression : public Expression {
public:
    std::unique_ptr<Expression> base; // For inner class creation
    std::unique_ptr<TypeName> type;
    std::vector<std::unique_ptr<Type>> typeArguments;
    std::vector<std::unique_ptr<Expression>> arguments;
    std::unique_ptr<class ClassBody> classBody; // For anonymous classes

    std::string kind() const override { return "ClassCreationExpression"; }
    static std::unique_ptr<ClassCreationExpression> fromJson(const nlohmann::json& j);
};

// new Type[dims]
class ArrayCreationExpression : public Expression {
public:
    std::unique_ptr<Type> elementType;
    std::vector<std::unique_ptr<Expression>> dimensionExprs;
    int extraDims = 0;
    std::unique_ptr<class ArrayInitializer> initializer;

    std::string kind() const override { return "ArrayCreationExpression"; }
    static std::unique_ptr<ArrayCreationExpression> fromJson(const nlohmann::json& j);
};

// {elem1, elem2, ...}
class ArrayInitializer : public Expression {
public:
    std::vector<std::unique_ptr<Expression>> elements;

    std::string kind() const override { return "ArrayInitializer"; }
    static std::unique_ptr<ArrayInitializer> fromJson(const nlohmann::json& j);
};

// Type.class
class ClassLiteralExpression : public Expression {
public:
    std::unique_ptr<Type> type;

    std::string kind() const override { return "ClassLiteral"; }
    static std::unique_ptr<ClassLiteralExpression> fromJson(const nlohmann::json& j);
};

// Base class for statements
class Statement : public Node {
public:
    static std::unique_ptr<Statement> fromJson(const nlohmann::json& j);
};

// Block statement { ... }
class BlockStatement : public Statement {
public:
    std::vector<std::unique_ptr<Statement>> statements;

    std::string kind() const override { return "Block"; }
    static std::unique_ptr<BlockStatement> fromJson(const nlohmann::json& j);
};

// Empty statement ;
class EmptyStatement : public Statement {
public:
    std::string kind() const override { return "EmptyStatement"; }
    static std::unique_ptr<EmptyStatement> fromJson(const nlohmann::json& j);
};

// Expression statement (expr;)
class ExpressionStatement : public Statement {
public:
    std::unique_ptr<Expression> expression;

    std::string kind() const override { return "ExpressionStatement"; }
    static std::unique_ptr<ExpressionStatement> fromJson(const nlohmann::json& j);
};

// Variable declarator (name = init)
class VariableDeclarator : public Node {
public:
    std::string name;
    int extraDims = 0;
    std::unique_ptr<Expression> initializer;

    std::string kind() const override { return "VariableDeclarator"; }
    static std::unique_ptr<VariableDeclarator> fromJson(const nlohmann::json& j);
};

// Local variable declaration statement
class LocalVariableStatement : public Statement {
public:
    std::unique_ptr<Modifiers> modifiers;
    std::unique_ptr<Type> type;
    std::vector<std::unique_ptr<VariableDeclarator>> declarators;

    std::string kind() const override { return "LocalVariableStatement"; }
    static std::unique_ptr<LocalVariableStatement> fromJson(const nlohmann::json& j);
};

// if statement
class IfStatement : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> thenStatement;
    std::unique_ptr<Statement> elseStatement;

    std::string kind() const override { return "IfStatement"; }
    static std::unique_ptr<IfStatement> fromJson(const nlohmann::json& j);
};

// while statement
class WhileStatement : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> body;

    std::string kind() const override { return "WhileStatement"; }
    static std::unique_ptr<WhileStatement> fromJson(const nlohmann::json& j);
};

// do-while statement
class DoStatement : public Statement {
public:
    std::unique_ptr<Statement> body;
    std::unique_ptr<Expression> condition;

    std::string kind() const override { return "DoStatement"; }
    static std::unique_ptr<DoStatement> fromJson(const nlohmann::json& j);
};

// for statement
class ForStatement : public Statement {
public:
    std::vector<std::unique_ptr<Statement>> init;
    std::unique_ptr<Expression> condition;
    std::vector<std::unique_ptr<Expression>> update;
    std::unique_ptr<Statement> body;

    std::string kind() const override { return "ForStatement"; }
    static std::unique_ptr<ForStatement> fromJson(const nlohmann::json& j);
};

// enhanced for statement (for-each)
class ForeachStatement : public Statement {
public:
    std::unique_ptr<Modifiers> modifiers;
    std::unique_ptr<Type> type;
    std::string variableName;
    std::unique_ptr<Expression> iterable;
    std::unique_ptr<Statement> body;

    std::string kind() const override { return "ForeachStatement"; }
    static std::unique_ptr<ForeachStatement> fromJson(const nlohmann::json& j);
};

// switch statement
class SwitchStatement : public Statement {
public:
    struct SwitchLabel {
        bool isDefault;
        std::unique_ptr<Expression> expression; // null for default
    };
    struct SwitchBlock {
        std::vector<SwitchLabel> labels;
        std::vector<std::unique_ptr<Statement>> statements;
    };

    std::unique_ptr<Expression> expression;
    std::vector<SwitchBlock> blocks;

    std::string kind() const override { return "SwitchStatement"; }
    static std::unique_ptr<SwitchStatement> fromJson(const nlohmann::json& j);
};

// break statement
class BreakStatement : public Statement {
public:
    std::optional<std::string> label;

    std::string kind() const override { return "BreakStatement"; }
    static std::unique_ptr<BreakStatement> fromJson(const nlohmann::json& j);
};

// continue statement
class ContinueStatement : public Statement {
public:
    std::optional<std::string> label;

    std::string kind() const override { return "ContinueStatement"; }
    static std::unique_ptr<ContinueStatement> fromJson(const nlohmann::json& j);
};

// return statement
class ReturnStatement : public Statement {
public:
    std::unique_ptr<Expression> expression;

    std::string kind() const override { return "ReturnStatement"; }
    static std::unique_ptr<ReturnStatement> fromJson(const nlohmann::json& j);
};

// throw statement
class ThrowStatement : public Statement {
public:
    std::unique_ptr<Expression> expression;

    std::string kind() const override { return "ThrowStatement"; }
    static std::unique_ptr<ThrowStatement> fromJson(const nlohmann::json& j);
};

// synchronized statement
class SynchronizedStatement : public Statement {
public:
    std::unique_ptr<Expression> expression;
    std::unique_ptr<BlockStatement> body;

    std::string kind() const override { return "SynchronizedStatement"; }
    static std::unique_ptr<SynchronizedStatement> fromJson(const nlohmann::json& j);
};

// try statement
class TryStatement : public Statement {
public:
    struct CatchClause {
        std::unique_ptr<Modifiers> modifiers;
        std::unique_ptr<Type> type;
        std::string variableName;
        std::unique_ptr<BlockStatement> block;
    };

    std::unique_ptr<BlockStatement> tryBlock;
    std::vector<CatchClause> catchClauses;
    std::unique_ptr<BlockStatement> finallyBlock;

    std::string kind() const override { return "TryStatement"; }
    static std::unique_ptr<TryStatement> fromJson(const nlohmann::json& j);
};

// assert statement
class AssertStatement : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Expression> message;

    std::string kind() const override { return "AssertStatement"; }
    static std::unique_ptr<AssertStatement> fromJson(const nlohmann::json& j);
};

// this() call
class ThisCall : public Statement {
public:
    std::vector<std::unique_ptr<Type>> typeArguments;
    std::vector<std::unique_ptr<Expression>> arguments;

    std::string kind() const override { return "ThisCall"; }
    static std::unique_ptr<ThisCall> fromJson(const nlohmann::json& j);
};

// super() call
class SuperCall : public Statement {
public:
    std::unique_ptr<Expression> base;
    std::vector<std::unique_ptr<Type>> typeArguments;
    std::vector<std::unique_ptr<Expression>> arguments;

    std::string kind() const override { return "SuperCall"; }
    static std::unique_ptr<SuperCall> fromJson(const nlohmann::json& j);
};

// Local class declaration as statement
class LocalClassStatement : public Statement {
public:
    std::unique_ptr<class ClassDeclaration> declaration;

    std::string kind() const override { return "LocalClassStatement"; }
    static std::unique_ptr<LocalClassStatement> fromJson(const nlohmann::json& j);
};

// Base class for declarations
class Declaration : public Node {
public:
    std::unique_ptr<Modifiers> modifiers;

    static std::unique_ptr<Declaration> fromJson(const nlohmann::json& j);
};

// Method body
class MethodBody : public Node {
public:
    std::unique_ptr<Statement> explicitConstructorCall; // this() or super()
    std::vector<std::unique_ptr<Statement>> statements;

    std::string kind() const override { return "MethodBody"; }
    static std::unique_ptr<MethodBody> fromJson(const nlohmann::json& j);
};

// Method/constructor parameter
class FormalParameter : public Node {
public:
    std::unique_ptr<Modifiers> modifiers;
    std::unique_ptr<Type> type;
    std::string name;
    int extraDims = 0;
    bool isVarargs = false;

    std::string kind() const override { return "FormalParameter"; }
    static std::unique_ptr<FormalParameter> fromJson(const nlohmann::json& j);
};

// Field declaration
class FieldDeclaration : public Declaration {
public:
    std::unique_ptr<Type> type;
    std::vector<std::unique_ptr<VariableDeclarator>> declarators;

    std::string kind() const override { return "FieldDeclaration"; }
    static std::unique_ptr<FieldDeclaration> fromJson(const nlohmann::json& j);
};

// Method declaration
class MethodDeclaration : public Declaration {
public:
    std::vector<std::unique_ptr<TypeParameter>> typeParameters;
    std::unique_ptr<Type> returnType;
    std::string name;
    std::vector<std::unique_ptr<FormalParameter>> parameters;
    int extraDims = 0;
    std::vector<std::unique_ptr<TypeName>> throwsTypes;
    std::unique_ptr<MethodBody> body; // null for abstract/interface methods
    std::unique_ptr<Expression> defaultValue; // For annotation methods

    std::string kind() const override { return "MethodDeclaration"; }
    static std::unique_ptr<MethodDeclaration> fromJson(const nlohmann::json& j);
};

// Constructor declaration
class ConstructorDeclaration : public Declaration {
public:
    std::vector<std::unique_ptr<TypeParameter>> typeParameters;
    std::string name;
    std::vector<std::unique_ptr<FormalParameter>> parameters;
    std::vector<std::unique_ptr<TypeName>> throwsTypes;
    std::unique_ptr<MethodBody> body;

    std::string kind() const override { return "ConstructorDeclaration"; }
    static std::unique_ptr<ConstructorDeclaration> fromJson(const nlohmann::json& j);
};

// Static/instance initializer
class InitializerDeclaration : public Declaration {
public:
    bool isStatic = false;
    std::unique_ptr<MethodBody> body;

    std::string kind() const override { return "InitializerDeclaration"; }
    static std::unique_ptr<InitializerDeclaration> fromJson(const nlohmann::json& j);
};

// Class body (shared by class, enum, interface, annotation, anonymous class)
class ClassBody : public Node {
public:
    std::vector<std::unique_ptr<Declaration>> declarations;

    std::string kind() const override { return "ClassBody"; }
    static std::unique_ptr<ClassBody> fromJson(const nlohmann::json& j);
};

// Class declaration
class ClassDeclaration : public Declaration {
public:
    std::string name;
    std::vector<std::unique_ptr<TypeParameter>> typeParameters;
    std::unique_ptr<TypeName> superclass;
    std::vector<std::unique_ptr<TypeName>> interfaces;
    std::unique_ptr<ClassBody> body;

    std::string kind() const override { return "ClassDeclaration"; }
    static std::unique_ptr<ClassDeclaration> fromJson(const nlohmann::json& j);
};

// Enum constant
class EnumConstant : public Declaration {
public:
    std::string name;
    std::vector<std::unique_ptr<Expression>> arguments;
    std::unique_ptr<ClassBody> body;

    std::string kind() const override { return "EnumConstant"; }
    static std::unique_ptr<EnumConstant> fromJson(const nlohmann::json& j);
};

// Enum declaration
class EnumDeclaration : public Declaration {
public:
    std::string name;
    std::vector<std::unique_ptr<TypeName>> interfaces;
    std::vector<std::unique_ptr<EnumConstant>> constants;
    std::unique_ptr<ClassBody> body;

    std::string kind() const override { return "EnumDeclaration"; }
    static std::unique_ptr<EnumDeclaration> fromJson(const nlohmann::json& j);
};

// Interface declaration
class InterfaceDeclaration : public Declaration {
public:
    std::string name;
    std::vector<std::unique_ptr<TypeParameter>> typeParameters;
    std::vector<std::unique_ptr<TypeName>> extendsTypes;
    std::unique_ptr<ClassBody> body;

    std::string kind() const override { return "InterfaceDeclaration"; }
    static std::unique_ptr<InterfaceDeclaration> fromJson(const nlohmann::json& j);
};

// Annotation type declaration
class AnnotationDeclaration : public Declaration {
public:
    std::string name;
    std::unique_ptr<ClassBody> body;

    std::string kind() const override { return "AnnotationDeclaration"; }
    static std::unique_ptr<AnnotationDeclaration> fromJson(const nlohmann::json& j);
};

// Empty declaration (;)
class EmptyDeclaration : public Declaration {
public:
    std::string kind() const override { return "EmptyDeclaration"; }
    static std::unique_ptr<EmptyDeclaration> fromJson(const nlohmann::json& j);
};

// Package declaration
class PackageDeclaration : public Node {
public:
    std::unique_ptr<Modifiers> modifiers; // Annotations
    std::unique_ptr<Name> name;

    std::string kind() const override { return "PackageDeclaration"; }
    static std::unique_ptr<PackageDeclaration> fromJson(const nlohmann::json& j);
};

// Import declaration
class ImportDeclaration : public Node {
public:
    std::unique_ptr<Name> name;
    bool isStatic = false;
    bool isOnDemand = false; // import x.y.* vs import x.y.Z

    std::string kind() const override { return "ImportDeclaration"; }
    static std::unique_ptr<ImportDeclaration> fromJson(const nlohmann::json& j);
};

// Compilation unit (one source file)
class CompilationUnit : public Node {
public:
    std::unique_ptr<PackageDeclaration> packageDecl;
    std::vector<std::unique_ptr<ImportDeclaration>> imports;
    std::vector<std::unique_ptr<Declaration>> types;

    std::string kind() const override { return "CompilationUnit"; }
    static std::unique_ptr<CompilationUnit> fromJson(const nlohmann::json& j);
};

} // namespace jast
} // namespace Jopa

#endif // jast_INCLUDED
