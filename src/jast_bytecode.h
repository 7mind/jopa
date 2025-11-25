#ifndef jast_bytecode_INCLUDED
#define jast_bytecode_INCLUDED

#include "platform.h"
#include "jast2.h"
#include "bytecode.h"
#include "control.h"
#include "symbol.h"

namespace Jopa {

class Control;
class TypeSymbol;
class MethodSymbol;
class Semantic;

// JastByteCode - bytecode generator that works with jast2 AST nodes
// This class bridges jast2 nodes to the existing ByteCode infrastructure
// by using the symbol links established during semantic analysis
class JastByteCode {
public:
    JastByteCode(Control& control, Semantic& semantic, TypeSymbol* type);
    ~JastByteCode();

    // Main entry point - generate bytecode for the type
    void GenerateCode();

    // Generate class file
    void GenerateClassFile();

private:
    Control& control_;
    Semantic& semantic_;
    TypeSymbol* unit_type_;

    // Helper to check if type uses jast2 AST
    bool usesJast2() const;

    // Process declarations using jast2 nodes
    void processJast2ClassBody(jast2::ClassBody* body);
    void processJast2EnumDeclaration(jast2::EnumDeclaration* decl);

    // Process members
    void processJast2Fields(jast2::ClassBody* body);
    void processJast2Methods(jast2::ClassBody* body);
    void processJast2Constructors(jast2::ClassBody* body);

    // Generate method bytecode
    void generateMethodCode(MethodSymbol* method, jast2::MethodBody* body);
    void generateConstructorCode(MethodSymbol* method, jast2::MethodBody* body);
    void generateInitializerCode(jast2::InitializerDeclaration* init, bool is_static);

    // Expression code generation
    void emitExpression(jast2::Expression* expr);
    void emitLiteral(jast2::LiteralExpression* expr);
    void emitNameExpression(jast2::NameExpression* expr);
    void emitBinaryExpression(jast2::BinaryExpression* expr);
    void emitUnaryExpression(jast2::UnaryExpression* expr);
    void emitAssignmentExpression(jast2::AssignmentExpression* expr);
    void emitConditionalExpression(jast2::ConditionalExpression* expr);
    void emitFieldAccess(jast2::FieldAccessExpression* expr);
    void emitMethodInvocation(jast2::MethodInvocationExpression* expr);
    void emitClassCreation(jast2::ClassCreationExpression* expr);
    void emitArrayCreation(jast2::ArrayCreationExpression* expr);
    void emitArrayAccess(jast2::ArrayAccessExpression* expr);
    void emitCastExpression(jast2::CastExpression* expr);
    void emitInstanceofExpression(jast2::InstanceofExpression* expr);

    // Statement code generation
    void emitStatement(jast2::Statement* stmt);
    void emitBlockStatement(jast2::BlockStatement* stmt);
    void emitExpressionStatement(jast2::ExpressionStatement* stmt);
    void emitLocalVariableStatement(jast2::LocalVariableStatement* stmt);
    void emitIfStatement(jast2::IfStatement* stmt);
    void emitWhileStatement(jast2::WhileStatement* stmt);
    void emitDoStatement(jast2::DoStatement* stmt);
    void emitForStatement(jast2::ForStatement* stmt);
    void emitForeachStatement(jast2::ForeachStatement* stmt);
    void emitSwitchStatement(jast2::SwitchStatement* stmt);
    void emitBreakStatement(jast2::BreakStatement* stmt);
    void emitContinueStatement(jast2::ContinueStatement* stmt);
    void emitReturnStatement(jast2::ReturnStatement* stmt);
    void emitThrowStatement(jast2::ThrowStatement* stmt);
    void emitTryStatement(jast2::TryStatement* stmt);
    void emitSynchronizedStatement(jast2::SynchronizedStatement* stmt);
    void emitAssertStatement(jast2::AssertStatement* stmt);

    // The underlying ByteCode generator (created when needed)
    ByteCode* bytecode_;
};

} // namespace Jopa

#endif // jast_bytecode_INCLUDED
