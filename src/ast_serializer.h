#ifndef ast_serializer_INCLUDED
#define ast_serializer_INCLUDED

#include "platform.h"
#include <nlohmann/json.hpp>

namespace Jopa {

// Forward declarations for legacy AST classes
class LexStream;
class Ast;
class AstBlock;
class AstName;
class AstPrimitiveType;
class AstBrackets;
class AstArrayType;
class AstWildcard;
class AstTypeArguments;
class AstTypeName;
class AstMemberValuePair;
class AstAnnotation;
class AstModifierKeyword;
class AstModifiers;
class AstPackageDeclaration;
class AstImportDeclaration;
class AstCompilationUnit;
class AstEmptyDeclaration;
class AstClassBody;
class AstTypeParameter;
class AstTypeParameters;
class AstClassDeclaration;
class AstArrayInitializer;
class AstVariableDeclaratorId;
class AstVariableDeclarator;
class AstFieldDeclaration;
class AstFormalParameter;
class AstMethodDeclarator;
class AstMethodBody;
class AstMethodDeclaration;
class AstInitializerDeclaration;
class AstArguments;
class AstThisCall;
class AstSuperCall;
class AstConstructorDeclaration;
class AstEnumDeclaration;
class AstEnumConstant;
class AstInterfaceDeclaration;
class AstAnnotationDeclaration;
class AstLocalVariableStatement;
class AstLocalClassStatement;
class AstIfStatement;
class AstEmptyStatement;
class AstExpressionStatement;
class AstSwitchLabel;
class AstSwitchBlockStatement;
class AstSwitchStatement;
class AstWhileStatement;
class AstDoStatement;
class AstForStatement;
class AstForeachStatement;
class AstBreakStatement;
class AstContinueStatement;
class AstReturnStatement;
class AstThrowStatement;
class AstSynchronizedStatement;
class AstAssertStatement;
class AstCatchClause;
class AstFinallyClause;
class AstTryStatement;
class AstIntegerLiteral;
class AstLongLiteral;
class AstFloatLiteral;
class AstDoubleLiteral;
class AstTrueLiteral;
class AstFalseLiteral;
class AstStringLiteral;
class AstCharacterLiteral;
class AstNullLiteral;
class AstClassLiteral;
class AstThisExpression;
class AstSuperExpression;
class AstParenthesizedExpression;
class AstClassCreationExpression;
class AstDimExpr;
class AstArrayCreationExpression;
class AstFieldAccess;
class AstMethodInvocation;
class AstArrayAccess;
class AstPostUnaryExpression;
class AstPreUnaryExpression;
class AstCastExpression;
class AstBinaryExpression;
class AstInstanceofExpression;
class AstConditionalExpression;
class AstAssignmentExpression;
class AstStatement;
class AstExpression;
class AstType;
class AstMemberValue;
class AstDeclared;
class AstDeclaredType;

// Serializes the legacy AST to JSON format
// This class provides the bridge between the legacy parser output
// and the new JSON-based AST representation
class AstSerializer {
public:
    explicit AstSerializer(LexStream& lex_stream);

    // Serialize a compilation unit to JSON
    nlohmann::json serialize(AstCompilationUnit* unit);

    // Serialize a package declaration to JSON
    nlohmann::json serialize(AstPackageDeclaration* decl);

private:
    LexStream& lex_stream_;

    // Helper to create location info from a token
    nlohmann::json tokenLocation(TokenIndex token);

    // Helper to get token text
    std::string tokenText(TokenIndex token);

    // Serialization methods for each AST node type
    nlohmann::json serializeAst(Ast* node);
    nlohmann::json serializeStatement(AstStatement* stmt);
    nlohmann::json serializeExpression(AstExpression* expr);
    nlohmann::json serializeType(AstType* type);
    nlohmann::json serializeMemberValue(AstMemberValue* value);
    nlohmann::json serializeDeclared(AstDeclared* decl);
    nlohmann::json serializeDeclaredType(AstDeclaredType* decl);

    // Specific node serializers
    nlohmann::json serializeBlock(AstBlock* block);
    nlohmann::json serializeName(AstName* name);
    nlohmann::json serializePrimitiveType(AstPrimitiveType* type);
    nlohmann::json serializeBrackets(AstBrackets* brackets);
    nlohmann::json serializeArrayType(AstArrayType* type);
    nlohmann::json serializeWildcard(AstWildcard* wildcard);
    nlohmann::json serializeTypeArguments(AstTypeArguments* args);
    nlohmann::json serializeTypeName(AstTypeName* type);
    nlohmann::json serializeMemberValuePair(AstMemberValuePair* pair);
    nlohmann::json serializeAnnotation(AstAnnotation* annotation);
    nlohmann::json serializeModifierKeyword(AstModifierKeyword* keyword);
    nlohmann::json serializeModifiers(AstModifiers* modifiers);
    nlohmann::json serializePackageDeclaration(AstPackageDeclaration* decl);
    nlohmann::json serializeImportDeclaration(AstImportDeclaration* decl);
    nlohmann::json serializeCompilationUnit(AstCompilationUnit* unit);
    nlohmann::json serializeEmptyDeclaration(AstEmptyDeclaration* decl);
    nlohmann::json serializeClassBody(AstClassBody* body);
    nlohmann::json serializeTypeParameter(AstTypeParameter* param);
    nlohmann::json serializeTypeParameters(AstTypeParameters* params);
    nlohmann::json serializeClassDeclaration(AstClassDeclaration* decl);
    nlohmann::json serializeArrayInitializer(AstArrayInitializer* init);
    nlohmann::json serializeVariableDeclaratorId(AstVariableDeclaratorId* id);
    nlohmann::json serializeVariableDeclarator(AstVariableDeclarator* decl);
    nlohmann::json serializeFieldDeclaration(AstFieldDeclaration* decl);
    nlohmann::json serializeFormalParameter(AstFormalParameter* param);
    nlohmann::json serializeMethodDeclarator(AstMethodDeclarator* decl);
    nlohmann::json serializeMethodBody(AstMethodBody* body);
    nlohmann::json serializeMethodDeclaration(AstMethodDeclaration* decl);
    nlohmann::json serializeInitializerDeclaration(AstInitializerDeclaration* decl);
    nlohmann::json serializeArguments(AstArguments* args);
    nlohmann::json serializeThisCall(AstThisCall* call);
    nlohmann::json serializeSuperCall(AstSuperCall* call);
    nlohmann::json serializeConstructorDeclaration(AstConstructorDeclaration* decl);
    nlohmann::json serializeEnumDeclaration(AstEnumDeclaration* decl);
    nlohmann::json serializeEnumConstant(AstEnumConstant* constant);
    nlohmann::json serializeInterfaceDeclaration(AstInterfaceDeclaration* decl);
    nlohmann::json serializeAnnotationDeclaration(AstAnnotationDeclaration* decl);
    nlohmann::json serializeLocalVariableStatement(AstLocalVariableStatement* stmt);
    nlohmann::json serializeLocalClassStatement(AstLocalClassStatement* stmt);
    nlohmann::json serializeIfStatement(AstIfStatement* stmt);
    nlohmann::json serializeEmptyStatement(AstEmptyStatement* stmt);
    nlohmann::json serializeExpressionStatement(AstExpressionStatement* stmt);
    nlohmann::json serializeSwitchLabel(AstSwitchLabel* label);
    nlohmann::json serializeSwitchBlockStatement(AstSwitchBlockStatement* stmt);
    nlohmann::json serializeSwitchStatement(AstSwitchStatement* stmt);
    nlohmann::json serializeWhileStatement(AstWhileStatement* stmt);
    nlohmann::json serializeDoStatement(AstDoStatement* stmt);
    nlohmann::json serializeForStatement(AstForStatement* stmt);
    nlohmann::json serializeForeachStatement(AstForeachStatement* stmt);
    nlohmann::json serializeBreakStatement(AstBreakStatement* stmt);
    nlohmann::json serializeContinueStatement(AstContinueStatement* stmt);
    nlohmann::json serializeReturnStatement(AstReturnStatement* stmt);
    nlohmann::json serializeThrowStatement(AstThrowStatement* stmt);
    nlohmann::json serializeSynchronizedStatement(AstSynchronizedStatement* stmt);
    nlohmann::json serializeAssertStatement(AstAssertStatement* stmt);
    nlohmann::json serializeCatchClause(AstCatchClause* clause);
    nlohmann::json serializeFinallyClause(AstFinallyClause* clause);
    nlohmann::json serializeTryStatement(AstTryStatement* stmt);
    nlohmann::json serializeIntegerLiteral(AstIntegerLiteral* lit);
    nlohmann::json serializeLongLiteral(AstLongLiteral* lit);
    nlohmann::json serializeFloatLiteral(AstFloatLiteral* lit);
    nlohmann::json serializeDoubleLiteral(AstDoubleLiteral* lit);
    nlohmann::json serializeTrueLiteral(AstTrueLiteral* lit);
    nlohmann::json serializeFalseLiteral(AstFalseLiteral* lit);
    nlohmann::json serializeStringLiteral(AstStringLiteral* lit);
    nlohmann::json serializeCharacterLiteral(AstCharacterLiteral* lit);
    nlohmann::json serializeNullLiteral(AstNullLiteral* lit);
    nlohmann::json serializeClassLiteral(AstClassLiteral* lit);
    nlohmann::json serializeThisExpression(AstThisExpression* expr);
    nlohmann::json serializeSuperExpression(AstSuperExpression* expr);
    nlohmann::json serializeParenthesizedExpression(AstParenthesizedExpression* expr);
    nlohmann::json serializeClassCreationExpression(AstClassCreationExpression* expr);
    nlohmann::json serializeDimExpr(AstDimExpr* expr);
    nlohmann::json serializeArrayCreationExpression(AstArrayCreationExpression* expr);
    nlohmann::json serializeFieldAccess(AstFieldAccess* expr);
    nlohmann::json serializeMethodInvocation(AstMethodInvocation* expr);
    nlohmann::json serializeArrayAccess(AstArrayAccess* expr);
    nlohmann::json serializePostUnaryExpression(AstPostUnaryExpression* expr);
    nlohmann::json serializePreUnaryExpression(AstPreUnaryExpression* expr);
    nlohmann::json serializeCastExpression(AstCastExpression* expr);
    nlohmann::json serializeBinaryExpression(AstBinaryExpression* expr);
    nlohmann::json serializeInstanceofExpression(AstInstanceofExpression* expr);
    nlohmann::json serializeConditionalExpression(AstConditionalExpression* expr);
    nlohmann::json serializeAssignmentExpression(AstAssignmentExpression* expr);
};

} // namespace Jopa

#endif // ast_serializer_INCLUDED
