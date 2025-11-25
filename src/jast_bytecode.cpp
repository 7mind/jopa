#include "jast_bytecode.h"
#include "semantic.h"
#include "stream.h"
#include "option.h"

namespace Jopa {

//=============================================================================
// CONSTRUCTOR / DESTRUCTOR
//=============================================================================

JastByteCode::JastByteCode(Control& control, Semantic& semantic, TypeSymbol* type)
    : control_(control)
    , semantic_(semantic)
    , unit_type_(type)
    , bytecode_(nullptr)
{
    // control_ and semantic_ are kept for future use when implementing
    // jast2-specific bytecode generation
    (void)control_;
    (void)semantic_;
}

JastByteCode::~JastByteCode() {
    delete bytecode_;
}

//=============================================================================
// MAIN ENTRY POINTS
//=============================================================================

bool JastByteCode::usesJast2() const {
    return unit_type_ && (
        unit_type_->jast2_class_decl ||
        unit_type_->jast2_interface_decl ||
        unit_type_->jast2_enum_decl ||
        unit_type_->jast2_annotation_decl
    );
}

void JastByteCode::GenerateCode() {
    if (!unit_type_) return;

    // If this type doesn't use jast2, fall back to legacy bytecode generation
    if (!usesJast2()) {
        bytecode_ = new ByteCode(unit_type_);
        bytecode_->GenerateCode();
        return;
    }

    // Create the underlying bytecode generator
    bytecode_ = new ByteCode(unit_type_);

    // For now, delegate to the standard GenerateCode which works with symbols
    // The symbols were populated during semantic analysis and point to jast2 nodes
    // Future work: implement jast2-specific bytecode generation
    bytecode_->GenerateCode();
}

void JastByteCode::GenerateClassFile() {
    if (bytecode_) {
        // The ByteCode class handles class file generation
        // This would be called after GenerateCode()
    }
}

//=============================================================================
// CLASS/INTERFACE BODY PROCESSING (for future jast2-specific implementation)
//=============================================================================

void JastByteCode::processJast2ClassBody(jast2::ClassBody* /*body*/) {
    // Future: implement jast2-specific class body processing
    // For now, the standard ByteCode::GenerateCode() handles this via symbols
}

void JastByteCode::processJast2EnumDeclaration(jast2::EnumDeclaration* /*decl*/) {
    // Future: implement jast2-specific enum processing
}

//=============================================================================
// MEMBER PROCESSING (for future jast2-specific implementation)
//=============================================================================

void JastByteCode::processJast2Fields(jast2::ClassBody* /*body*/) {
    // Future: implement jast2-specific field processing
}

void JastByteCode::processJast2Methods(jast2::ClassBody* /*body*/) {
    // Future: implement jast2-specific method processing
}

void JastByteCode::processJast2Constructors(jast2::ClassBody* /*body*/) {
    // Future: implement jast2-specific constructor processing
}

//=============================================================================
// METHOD/CONSTRUCTOR CODE GENERATION
//=============================================================================

void JastByteCode::generateMethodCode(MethodSymbol* method, jast2::MethodBody* body) {
    if (!method || !body) return;

    // The actual bytecode generation uses the existing ByteCode infrastructure
    // which works with symbols. We just need to emit the statements.

    // Emit explicit constructor call if present (for constructors called from methods?)
    if (body->explicitConstructorCall) {
        emitStatement(body->explicitConstructorCall.get());
    }

    // Emit method body statements
    for (auto& stmt : body->statements) {
        emitStatement(stmt.get());
    }
}

void JastByteCode::generateConstructorCode(MethodSymbol* method, jast2::MethodBody* body) {
    if (!method || !body) return;

    // Emit explicit constructor call (this() or super())
    if (body->explicitConstructorCall) {
        emitStatement(body->explicitConstructorCall.get());
    }

    // Emit constructor body statements
    for (auto& stmt : body->statements) {
        emitStatement(stmt.get());
    }
}

void JastByteCode::generateInitializerCode(jast2::InitializerDeclaration* init, bool /*is_static*/) {
    if (!init || !init->body) return;

    for (auto& stmt : init->body->statements) {
        emitStatement(stmt.get());
    }
}

//=============================================================================
// EXPRESSION CODE GENERATION
//=============================================================================

void JastByteCode::emitExpression(jast2::Expression* expr) {
    if (!expr) return;

    if (auto* lit = dynamic_cast<jast2::LiteralExpression*>(expr)) {
        emitLiteral(lit);
    } else if (auto* name = dynamic_cast<jast2::NameExpression*>(expr)) {
        emitNameExpression(name);
    } else if (auto* binary = dynamic_cast<jast2::BinaryExpression*>(expr)) {
        emitBinaryExpression(binary);
    } else if (auto* unary = dynamic_cast<jast2::UnaryExpression*>(expr)) {
        emitUnaryExpression(unary);
    } else if (auto* assign = dynamic_cast<jast2::AssignmentExpression*>(expr)) {
        emitAssignmentExpression(assign);
    } else if (auto* cond = dynamic_cast<jast2::ConditionalExpression*>(expr)) {
        emitConditionalExpression(cond);
    } else if (auto* field = dynamic_cast<jast2::FieldAccessExpression*>(expr)) {
        emitFieldAccess(field);
    } else if (auto* method = dynamic_cast<jast2::MethodInvocationExpression*>(expr)) {
        emitMethodInvocation(method);
    } else if (auto* create = dynamic_cast<jast2::ClassCreationExpression*>(expr)) {
        emitClassCreation(create);
    } else if (auto* arr_create = dynamic_cast<jast2::ArrayCreationExpression*>(expr)) {
        emitArrayCreation(arr_create);
    } else if (auto* arr_access = dynamic_cast<jast2::ArrayAccessExpression*>(expr)) {
        emitArrayAccess(arr_access);
    } else if (auto* cast = dynamic_cast<jast2::CastExpression*>(expr)) {
        emitCastExpression(cast);
    } else if (auto* instof = dynamic_cast<jast2::InstanceofExpression*>(expr)) {
        emitInstanceofExpression(instof);
    } else if (auto* paren = dynamic_cast<jast2::ParenthesizedExpression*>(expr)) {
        emitExpression(paren->expression.get());
    }
    // TODO: Handle remaining expression types
}

void JastByteCode::emitLiteral(jast2::LiteralExpression* expr) {
    if (!expr) return;
    // TODO: Implement literal loading
    // Uses bytecode_->LoadLiteral() or similar
    (void)expr;
}

void JastByteCode::emitNameExpression(jast2::NameExpression* expr) {
    if (!expr) return;
    // TODO: Implement name resolution and loading
    // If it's a variable, load it; if it's a type, handle appropriately
    (void)expr;
}

void JastByteCode::emitBinaryExpression(jast2::BinaryExpression* expr) {
    if (!expr) return;

    // Emit left operand
    emitExpression(expr->left.get());

    // Emit right operand
    emitExpression(expr->right.get());

    // TODO: Emit appropriate binary operation based on expr->op
    // Uses bytecode_->PutOp() with appropriate opcode
}

void JastByteCode::emitUnaryExpression(jast2::UnaryExpression* expr) {
    if (!expr) return;

    emitExpression(expr->operand.get());

    // TODO: Emit appropriate unary operation based on expr->op
}

void JastByteCode::emitAssignmentExpression(jast2::AssignmentExpression* expr) {
    if (!expr) return;

    // TODO: Handle assignment
    // Need to emit target address, value, and store instruction
    emitExpression(expr->left.get());
    emitExpression(expr->right.get());
}

void JastByteCode::emitConditionalExpression(jast2::ConditionalExpression* expr) {
    if (!expr) return;

    // TODO: Implement ternary operator with branch labels
    emitExpression(expr->condition.get());
    emitExpression(expr->thenExpr.get());
    emitExpression(expr->elseExpr.get());
}

void JastByteCode::emitFieldAccess(jast2::FieldAccessExpression* expr) {
    if (!expr) return;

    // TODO: Emit object reference and field access
    emitExpression(expr->base.get());
}

void JastByteCode::emitMethodInvocation(jast2::MethodInvocationExpression* expr) {
    if (!expr) return;

    // Emit base expression if present
    if (expr->base) {
        emitExpression(expr->base.get());
    }

    // Emit arguments
    for (auto& arg : expr->arguments) {
        emitExpression(arg.get());
    }

    // TODO: Emit invoke instruction based on method symbol
}

void JastByteCode::emitClassCreation(jast2::ClassCreationExpression* expr) {
    if (!expr) return;

    // TODO: Emit new, dup, arguments, invokespecial <init>
    for (auto& arg : expr->arguments) {
        emitExpression(arg.get());
    }
}

void JastByteCode::emitArrayCreation(jast2::ArrayCreationExpression* expr) {
    if (!expr) return;

    // Emit dimension expressions
    for (auto& dim : expr->dimensionExprs) {
        emitExpression(dim.get());
    }

    // TODO: Emit newarray/anewarray/multianewarray
}

void JastByteCode::emitArrayAccess(jast2::ArrayAccessExpression* expr) {
    if (!expr) return;

    emitExpression(expr->array.get());
    emitExpression(expr->index.get());

    // TODO: Emit array load instruction
}

void JastByteCode::emitCastExpression(jast2::CastExpression* expr) {
    if (!expr) return;

    emitExpression(expr->expression.get());

    // TODO: Emit checkcast or conversion instruction
}

void JastByteCode::emitInstanceofExpression(jast2::InstanceofExpression* expr) {
    if (!expr) return;

    emitExpression(expr->expression.get());

    // TODO: Emit instanceof instruction
}

//=============================================================================
// STATEMENT CODE GENERATION
//=============================================================================

void JastByteCode::emitStatement(jast2::Statement* stmt) {
    if (!stmt) return;

    if (auto* block = dynamic_cast<jast2::BlockStatement*>(stmt)) {
        emitBlockStatement(block);
    } else if (auto* expr_stmt = dynamic_cast<jast2::ExpressionStatement*>(stmt)) {
        emitExpressionStatement(expr_stmt);
    } else if (auto* local_var = dynamic_cast<jast2::LocalVariableStatement*>(stmt)) {
        emitLocalVariableStatement(local_var);
    } else if (auto* if_stmt = dynamic_cast<jast2::IfStatement*>(stmt)) {
        emitIfStatement(if_stmt);
    } else if (auto* while_stmt = dynamic_cast<jast2::WhileStatement*>(stmt)) {
        emitWhileStatement(while_stmt);
    } else if (auto* do_stmt = dynamic_cast<jast2::DoStatement*>(stmt)) {
        emitDoStatement(do_stmt);
    } else if (auto* for_stmt = dynamic_cast<jast2::ForStatement*>(stmt)) {
        emitForStatement(for_stmt);
    } else if (auto* foreach_stmt = dynamic_cast<jast2::ForeachStatement*>(stmt)) {
        emitForeachStatement(foreach_stmt);
    } else if (auto* switch_stmt = dynamic_cast<jast2::SwitchStatement*>(stmt)) {
        emitSwitchStatement(switch_stmt);
    } else if (auto* break_stmt = dynamic_cast<jast2::BreakStatement*>(stmt)) {
        emitBreakStatement(break_stmt);
    } else if (auto* continue_stmt = dynamic_cast<jast2::ContinueStatement*>(stmt)) {
        emitContinueStatement(continue_stmt);
    } else if (auto* return_stmt = dynamic_cast<jast2::ReturnStatement*>(stmt)) {
        emitReturnStatement(return_stmt);
    } else if (auto* throw_stmt = dynamic_cast<jast2::ThrowStatement*>(stmt)) {
        emitThrowStatement(throw_stmt);
    } else if (auto* try_stmt = dynamic_cast<jast2::TryStatement*>(stmt)) {
        emitTryStatement(try_stmt);
    } else if (auto* sync_stmt = dynamic_cast<jast2::SynchronizedStatement*>(stmt)) {
        emitSynchronizedStatement(sync_stmt);
    } else if (auto* assert_stmt = dynamic_cast<jast2::AssertStatement*>(stmt)) {
        emitAssertStatement(assert_stmt);
    }
    // TODO: Handle remaining statement types (this/super calls, labeled, etc.)
}

void JastByteCode::emitBlockStatement(jast2::BlockStatement* stmt) {
    if (!stmt) return;

    for (auto& s : stmt->statements) {
        emitStatement(s.get());
    }
}

void JastByteCode::emitExpressionStatement(jast2::ExpressionStatement* stmt) {
    if (!stmt) return;

    emitExpression(stmt->expression.get());

    // TODO: Pop result if expression has a value that's not used
}

void JastByteCode::emitLocalVariableStatement(jast2::LocalVariableStatement* stmt) {
    if (!stmt) return;

    // Process each declarator
    for (auto& var : stmt->declarators) {
        if (var->initializer) {
            emitExpression(var->initializer.get());
            // TODO: Store to local variable slot
        }
    }
}

void JastByteCode::emitIfStatement(jast2::IfStatement* stmt) {
    if (!stmt) return;

    // TODO: Emit condition with branch labels
    emitExpression(stmt->condition.get());
    emitStatement(stmt->thenStatement.get());
    if (stmt->elseStatement) {
        emitStatement(stmt->elseStatement.get());
    }
}

void JastByteCode::emitWhileStatement(jast2::WhileStatement* stmt) {
    if (!stmt) return;

    // TODO: Emit loop with continue/break labels
    emitExpression(stmt->condition.get());
    emitStatement(stmt->body.get());
}

void JastByteCode::emitDoStatement(jast2::DoStatement* stmt) {
    if (!stmt) return;

    // TODO: Emit do-while loop
    emitStatement(stmt->body.get());
    emitExpression(stmt->condition.get());
}

void JastByteCode::emitForStatement(jast2::ForStatement* stmt) {
    if (!stmt) return;

    // TODO: Emit for loop
    for (auto& s : stmt->init) {
        emitStatement(s.get());
    }
    if (stmt->condition) {
        emitExpression(stmt->condition.get());
    }
    emitStatement(stmt->body.get());
    for (auto& expr : stmt->update) {
        emitExpression(expr.get());
    }
}

void JastByteCode::emitForeachStatement(jast2::ForeachStatement* stmt) {
    if (!stmt) return;

    // TODO: Emit enhanced for loop (iterator or array iteration)
    emitExpression(stmt->iterable.get());
    emitStatement(stmt->body.get());
}

void JastByteCode::emitSwitchStatement(jast2::SwitchStatement* stmt) {
    if (!stmt) return;

    // TODO: Emit tableswitch or lookupswitch
    emitExpression(stmt->expression.get());
    for (auto& block : stmt->blocks) {
        for (auto& s : block.statements) {
            emitStatement(s.get());
        }
    }
}

void JastByteCode::emitBreakStatement(jast2::BreakStatement* /*stmt*/) {
    // TODO: Emit goto to break label
}

void JastByteCode::emitContinueStatement(jast2::ContinueStatement* /*stmt*/) {
    // TODO: Emit goto to continue label
}

void JastByteCode::emitReturnStatement(jast2::ReturnStatement* stmt) {
    if (!stmt) return;

    if (stmt->expression) {
        emitExpression(stmt->expression.get());
    }

    // TODO: Emit return instruction (ireturn, areturn, return, etc.)
}

void JastByteCode::emitThrowStatement(jast2::ThrowStatement* stmt) {
    if (!stmt) return;

    emitExpression(stmt->expression.get());

    // TODO: Emit athrow
}

void JastByteCode::emitTryStatement(jast2::TryStatement* stmt) {
    if (!stmt) return;

    // TODO: Emit try block with exception handlers
    emitStatement(stmt->tryBlock.get());

    for (auto& catch_clause : stmt->catchClauses) {
        emitStatement(catch_clause.block.get());
    }

    if (stmt->finallyBlock) {
        emitStatement(stmt->finallyBlock.get());
    }
}

void JastByteCode::emitSynchronizedStatement(jast2::SynchronizedStatement* stmt) {
    if (!stmt) return;

    // TODO: Emit monitorenter, body, monitorexit with exception handler
    emitExpression(stmt->expression.get());
    emitStatement(stmt->body.get());
}

void JastByteCode::emitAssertStatement(jast2::AssertStatement* stmt) {
    if (!stmt) return;

    // TODO: Emit assertion check if assertions enabled
    emitExpression(stmt->condition.get());
    if (stmt->message) {
        emitExpression(stmt->message.get());
    }
}

} // namespace Jopa
