#ifndef ast_json_INCLUDED
#define ast_json_INCLUDED

#include "platform.h"
#include <nlohmann/json.hpp>
#include <string>

namespace Jopa {

class Ast;
class AstCompilationUnit;
class LexStream;

using json = nlohmann::json;

// Serialize an AST node to JSON
// This converts the legacy parser's AST representation to a portable JSON format
class AstJsonSerializer
{
public:
    explicit AstJsonSerializer(LexStream* lex_stream);

    // Serialize an entire compilation unit
    json Serialize(AstCompilationUnit* compilation_unit);

    // Serialize any AST node (dispatches based on node kind)
    json SerializeNode(Ast* node);

    // Serialize lexical errors from LexStream
    static json SerializeLexErrors(LexStream* lex_stream);

private:
    LexStream* lex_stream_;

    // Helper to get source location for a token
    json TokenLocation(unsigned token_index);

    // Helper to get token text
    std::string TokenText(unsigned token_index);

    // Serialization methods for each node type
    json SerializeCompilationUnit(AstCompilationUnit* node);
    json SerializePackageDeclaration(class AstPackageDeclaration* node);
    json SerializeImportDeclaration(class AstImportDeclaration* node);

    // Type declarations
    json SerializeClassDeclaration(class AstClassDeclaration* node);
    json SerializeInterfaceDeclaration(class AstInterfaceDeclaration* node);
    json SerializeEnumDeclaration(class AstEnumDeclaration* node);
    json SerializeAnnotationDeclaration(class AstAnnotationDeclaration* node);
    json SerializeEmptyDeclaration(class AstEmptyDeclaration* node);
    json SerializeClassBody(class AstClassBody* node);
    json SerializeEnumConstant(class AstEnumConstant* node);

    // Type references
    json SerializeType(class AstType* node);
    json SerializePrimitiveType(class AstPrimitiveType* node);
    json SerializeTypeName(class AstTypeName* node);
    json SerializeArrayType(class AstArrayType* node);
    json SerializeWildcard(class AstWildcard* node);
    json SerializeTypeArguments(class AstTypeArguments* node);
    json SerializeTypeParameter(class AstTypeParameter* node);
    json SerializeTypeParameters(class AstTypeParameters* node);

    // Members
    json SerializeFieldDeclaration(class AstFieldDeclaration* node);
    json SerializeMethodDeclaration(class AstMethodDeclaration* node);
    json SerializeConstructorDeclaration(class AstConstructorDeclaration* node);
    json SerializeInitializerDeclaration(class AstInitializerDeclaration* node);
    json SerializeFormalParameter(class AstFormalParameter* node);
    json SerializeVariableDeclarator(class AstVariableDeclarator* node);
    json SerializeVariableDeclaratorId(class AstVariableDeclaratorId* node);
    json SerializeMethodDeclarator(class AstMethodDeclarator* node);
    json SerializeMethodBody(class AstMethodBody* node);

    // Modifiers and annotations
    json SerializeModifiers(class AstModifiers* node);
    json SerializeAnnotation(class AstAnnotation* node);
    json SerializeMemberValuePair(class AstMemberValuePair* node);

    // Statements
    json SerializeBlock(class AstBlock* node);
    json SerializeLocalVariableStatement(class AstLocalVariableStatement* node);
    json SerializeLocalClassStatement(class AstLocalClassStatement* node);
    json SerializeIfStatement(class AstIfStatement* node);
    json SerializeEmptyStatement(class AstEmptyStatement* node);
    json SerializeExpressionStatement(class AstExpressionStatement* node);
    json SerializeSwitchStatement(class AstSwitchStatement* node);
    json SerializeSwitchBlockStatement(class AstSwitchBlockStatement* node);
    json SerializeSwitchLabel(class AstSwitchLabel* node);
    json SerializeWhileStatement(class AstWhileStatement* node);
    json SerializeDoStatement(class AstDoStatement* node);
    json SerializeForStatement(class AstForStatement* node);
    json SerializeForeachStatement(class AstForeachStatement* node);
    json SerializeBreakStatement(class AstBreakStatement* node);
    json SerializeContinueStatement(class AstContinueStatement* node);
    json SerializeReturnStatement(class AstReturnStatement* node);
    json SerializeThrowStatement(class AstThrowStatement* node);
    json SerializeSynchronizedStatement(class AstSynchronizedStatement* node);
    json SerializeAssertStatement(class AstAssertStatement* node);
    json SerializeTryStatement(class AstTryStatement* node);
    json SerializeCatchClause(class AstCatchClause* node);
    json SerializeFinallyClause(class AstFinallyClause* node);

    // Expressions
    json SerializeExpression(class AstExpression* node);
    json SerializeName(class AstName* node);
    json SerializeFieldAccess(class AstFieldAccess* node);
    json SerializeIntegerLiteral(class AstIntegerLiteral* node);
    json SerializeLongLiteral(class AstLongLiteral* node);
    json SerializeFloatLiteral(class AstFloatLiteral* node);
    json SerializeDoubleLiteral(class AstDoubleLiteral* node);
    json SerializeTrueLiteral(class AstTrueLiteral* node);
    json SerializeFalseLiteral(class AstFalseLiteral* node);
    json SerializeStringLiteral(class AstStringLiteral* node);
    json SerializeCharacterLiteral(class AstCharacterLiteral* node);
    json SerializeNullLiteral(class AstNullLiteral* node);
    json SerializeClassLiteral(class AstClassLiteral* node);
    json SerializeThisExpression(class AstThisExpression* node);
    json SerializeSuperExpression(class AstSuperExpression* node);
    json SerializeParenthesizedExpression(class AstParenthesizedExpression* node);
    json SerializeArrayAccess(class AstArrayAccess* node);
    json SerializeMethodInvocation(class AstMethodInvocation* node);
    json SerializeClassCreationExpression(class AstClassCreationExpression* node);
    json SerializeArrayCreationExpression(class AstArrayCreationExpression* node);
    json SerializeDimExpr(class AstDimExpr* node);
    json SerializePostUnaryExpression(class AstPostUnaryExpression* node);
    json SerializePreUnaryExpression(class AstPreUnaryExpression* node);
    json SerializeCastExpression(class AstCastExpression* node);
    json SerializeBinaryExpression(class AstBinaryExpression* node);
    json SerializeInstanceofExpression(class AstInstanceofExpression* node);
    json SerializeConditionalExpression(class AstConditionalExpression* node);
    json SerializeAssignmentExpression(class AstAssignmentExpression* node);
    json SerializeArrayInitializer(class AstArrayInitializer* node);
    json SerializeThisCall(class AstThisCall* node);
    json SerializeSuperCall(class AstSuperCall* node);
    json SerializeArguments(class AstArguments* node);
    json SerializeBrackets(class AstBrackets* node);

    // Utility for converting AstKind to string
    static const char* KindToString(int kind);
};

// Get the string name for an AST node kind
const char* AstKindName(int kind);

} // namespace Jopa

#endif // ast_json_INCLUDED
