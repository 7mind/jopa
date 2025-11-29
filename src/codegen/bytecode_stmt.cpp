// ByteCode statement emission
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

bool ByteCode::EmitStatement(AstStatement* statement)
{
    if (! statement -> BlockCast())
    {
        line_number_table_attribute ->
            AddLineNumber(code_attribute -> CodeLength(),
                          semantic.lex_stream -> Line(statement -> LeftToken()));
    }

    assert(stack_depth == 0); // stack empty at start of statement

    switch (statement -> kind)
    {
    case Ast::METHOD_BODY:
    case Ast::BLOCK: // JLS 14.2
        return EmitBlockStatement((AstBlock*) statement);
    case Ast::LOCAL_VARIABLE_DECLARATION: // JLS 14.3
        {
            AstLocalVariableStatement* lvs =
                statement -> LocalVariableStatementCast();
            for (unsigned i = 0; i < lvs -> NumVariableDeclarators(); i++)
                DeclareLocalVariable(lvs -> VariableDeclarator(i));
        }
        return false;
    case Ast::EMPTY_STATEMENT: // JLS 14.5
        return false;
    case Ast::EXPRESSION_STATEMENT: // JLS 14.7
        EmitStatementExpression(statement -> ExpressionStatementCast() ->
                                expression);
        return false;
    case Ast::IF: // JLS 14.8
        {
            AstIfStatement* if_statement = (AstIfStatement*) statement;
            // Constant condition.
            if (IsOne(if_statement -> expression))
                return EmitBlockStatement(if_statement -> true_statement);
            if (IsZero(if_statement -> expression))
            {
                if (if_statement -> false_statement_opt)
                    return EmitBlockStatement(if_statement ->
                                              false_statement_opt);
                return false;
            }
            // True and false parts.
            if (if_statement -> false_statement_opt &&
                ! IsNop(if_statement -> false_statement_opt))
            {
                if (IsNop(if_statement -> true_statement))
                {
                    Label label;
                    EmitBranchIfExpression(if_statement -> expression,
                                           true, label,
                                           (if_statement ->
                                            false_statement_opt));
                    assert(stack_depth == 0);
                    EmitBlockStatement(if_statement -> false_statement_opt);
                    DefineLabel(label);
                    CompleteLabel(label);
                    return false;
                }
                Label label1,
                      label2;
                bool abrupt;
                AstBlock* true_statement = if_statement -> true_statement;
                EmitBranchIfExpression(if_statement -> expression,
                                       false, label1, true_statement);
                assert(stack_depth == 0);

                abrupt = EmitBlockStatement(true_statement);
                if (! abrupt)
                    EmitBranch(OP_GOTO, label2,
                               if_statement -> false_statement_opt);

                DefineLabel(label1);
                abrupt &= EmitBlockStatement(if_statement ->
                                             false_statement_opt);

                if (! abrupt)
                {
                    DefineLabel(label2);
                    CompleteLabel(label2);
                }
                CompleteLabel(label1);
                return abrupt;
            }
            // No false part.
            if (IsNop(if_statement -> true_statement))
            {
                EmitExpression(if_statement -> expression, false);
                return false;
            }
            Label label1;
            EmitBranchIfExpression(if_statement -> expression,
                                   false, label1,
                                   if_statement -> true_statement);
            assert(stack_depth == 0);
            EmitBlockStatement(if_statement -> true_statement);
            DefineLabel(label1);
            CompleteLabel(label1);
            return false;
        }
    case Ast::SWITCH: // JLS 14.9
        return EmitSwitchStatement(statement -> SwitchStatementCast());
    case Ast::SWITCH_BLOCK: // JLS 14.9
    case Ast::SWITCH_LABEL:
        //
        // These nodes are handled by SwitchStatement and
        // are not directly visited.
        //
        assert(false && "faulty logic encountered");
        return false;
    case Ast::WHILE: // JLS 14.10
        {
            AstWhileStatement* wp = statement -> WhileStatementCast();
            bool abrupt = false;
            //
            // Branch to continuation test. This test is placed after the
            // body of the loop we can fall through into it after each
            // loop iteration without the need for an additional branch,
            // unless the loop body always completes abruptly.
            //
            if (! wp -> statement -> can_complete_normally)
            {
                if (wp -> expression -> IsConstant())
                {
                    // must be true, or internal statement would be
                    // unreachable
                    assert(semantic.IsConstantTrue(wp -> expression));
                    abrupt = true;
                }
                else
                {
                    line_number_table_attribute ->
                        AddLineNumber(code_attribute -> CodeLength(),
                                      semantic.lex_stream -> Line(wp -> expression -> LeftToken()));
                    EmitBranchIfExpression(wp -> expression, false,
                                           method_stack -> TopBreakLabel(),
                                           wp -> statement);
                }
                EmitBlockStatement(wp -> statement);
                assert(stack_depth == 0);
                return abrupt;
            }
            Label& continue_label = method_stack -> TopContinueLabel();
            // Save locals state before loop body for StackMapTable generation
            // This is needed because inner-scope variables declared in the loop body
            // should not be included in the continue_label's frame.
            Tuple<StackMapGenerator::VerificationType>* saved_loop_locals = NULL;
            if (stack_map_generator)
                saved_loop_locals = stack_map_generator->SaveLocals();

            if (wp -> expression -> IsConstant())
            {
                // must be true, or internal statement would be
                // unreachable
                assert(semantic.IsConstantTrue(wp -> expression));
                abrupt = true;
            }
            else
                EmitBranch(OP_GOTO, continue_label, wp -> statement);
            Label begin_label;
            DefineLabel(begin_label);
            u2 begin_pc = code_attribute -> CodeLength();
            abrupt |= EmitBlockStatement(wp -> statement);
            bool empty = (begin_pc == code_attribute -> CodeLength());

            // For continue_label, record frame with pre-loop-body locals
            // This ensures inner-scope variables don't pollute the frame
            if (stack_map_generator && saved_loop_locals)
            {
                stack_map_generator->RecordFrameWithLocals(code_attribute->CodeLength(), saved_loop_locals);
                delete saved_loop_locals;
                saved_loop_locals = NULL;
            }
            DefineLabel(continue_label);
            assert(stack_depth == 0);

            //
            // Reset the line number before evaluating the expression
            //
            line_number_table_attribute ->
                AddLineNumber(code_attribute -> CodeLength(),
                              semantic.lex_stream -> Line(wp -> expression -> LeftToken()));

            EmitBranchIfExpression(wp -> expression, true,
                                   empty ? continue_label : begin_label,
                                   wp -> statement);
            CompleteLabel(begin_label);
            CompleteLabel(continue_label);
            return abrupt && ! wp -> can_complete_normally;
        }
    case Ast::DO: // JLS 14.11
        {
            AstDoStatement* sp = statement -> DoStatementCast();
            Label begin_label;
            DefineLabel(begin_label);
            bool abrupt = EmitBlockStatement(sp -> statement);
            if (IsLabelUsed(method_stack -> TopContinueLabel()))
            {
                DefineLabel(method_stack -> TopContinueLabel());
                CompleteLabel(method_stack -> TopContinueLabel());
                abrupt = false;
            }
            assert(stack_depth == 0);

            if (! abrupt)
            {
                //
                // Reset the line number before evaluating the expression
                //
                line_number_table_attribute ->
                    AddLineNumber(code_attribute -> CodeLength(),
                                  semantic.lex_stream -> Line(sp -> expression -> LeftToken()));
                EmitBranchIfExpression(sp -> expression, true,
                                       begin_label, sp -> statement);
            }
            CompleteLabel(begin_label);
            return (abrupt || IsOne(sp -> expression)) &&
                ! sp -> can_complete_normally;
        }
    case Ast::FOR: // JLS 14.12
        {
            AstForStatement* for_statement = statement -> ForStatementCast();
            bool abrupt = false;

            // Save locals state BEFORE for-init to restore when loop ends.
            // This is needed for StackMapTable - break/continue targets after the loop
            // should not include for-init variables.
            Tuple<StackMapGenerator::VerificationType>* saved_pre_loop_locals = NULL;
            if (stack_map_generator)
                saved_pre_loop_locals = stack_map_generator->SaveLocals();

            for (unsigned i = 0; i < for_statement -> NumForInitStatements(); i++)
                EmitStatement(for_statement -> ForInitStatement(i));
            Label begin_label;
            Label test_label;

            // Save locals state after for-init but before loop body for StackMapTable generation
            // This is needed because inner-scope variables declared in the loop body
            // should not be included in the continue_label/test_label's frame.
            Tuple<StackMapGenerator::VerificationType>* saved_loop_locals = NULL;
            if (stack_map_generator)
                saved_loop_locals = stack_map_generator->SaveLocals();

            //
            // The loop test is placed after the body, unless the body
            // always completes abruptly, to save an additional jump.
            //
            if (! for_statement -> statement -> can_complete_normally)
            {
                abrupt = true;
                if (for_statement -> end_expression_opt)
                {
                    if (for_statement -> end_expression_opt -> IsConstant())
                    {
                        // must be true, or internal statement would be
                        // unreachable
                        assert(semantic.IsConstantTrue(for_statement -> end_expression_opt));
                    }
                    else
                    {
                        abrupt = false;
                        line_number_table_attribute ->
                            AddLineNumber(code_attribute -> CodeLength(),
                                          semantic.lex_stream -> Line(for_statement -> end_expression_opt -> LeftToken()));
                        EmitBranchIfExpression(for_statement -> end_expression_opt,
                                               false,
                                               method_stack -> TopBreakLabel(),
                                               for_statement -> statement);
                    }
                }
                EmitBlockStatement(for_statement -> statement);
                assert(stack_depth == 0);
                // Restore pre-loop locals before returning
                if (stack_map_generator && saved_pre_loop_locals)
                {
                    stack_map_generator->RestoreLocals(saved_pre_loop_locals);
                    delete saved_pre_loop_locals;
                }
                delete saved_loop_locals;
                return abrupt;
            }
            Label& continue_label = method_stack -> TopContinueLabel();
            if (for_statement -> end_expression_opt &&
                ! for_statement -> end_expression_opt -> IsConstant())
            {
                EmitBranch(OP_GOTO,
                           (for_statement -> NumForUpdateStatements()
                            ? test_label : continue_label),
                           for_statement -> statement);
            }
            else
                abrupt = true;
            DefineLabel(begin_label);
            u2 begin_pc = code_attribute -> CodeLength();
            abrupt |= EmitBlockStatement(for_statement -> statement);
            bool empty = (begin_pc == code_attribute -> CodeLength());

            // For continue_label, record frame with pre-loop-body locals
            if (stack_map_generator && saved_loop_locals)
            {
                stack_map_generator->RecordFrameWithLocals(code_attribute->CodeLength(), saved_loop_locals);
            }
            DefineLabel(continue_label);
            for (unsigned j = 0;
                 j < for_statement -> NumForUpdateStatements(); j++)
            {
                EmitStatement(for_statement -> ForUpdateStatement(j));
            }
            // For test_label, also record frame with pre-loop-body locals
            // This is needed when test_label is a jump target from before the loop body
            if (stack_map_generator && saved_loop_locals)
            {
                stack_map_generator->RecordFrameWithLocals(code_attribute->CodeLength(), saved_loop_locals);
            }
            DefineLabel(test_label);
            CompleteLabel(test_label);

            AstExpression* end_expr = for_statement -> end_expression_opt;
            if (end_expr)
            {
                assert(stack_depth == 0);

                //
                // Reset the line number before evaluating the expression
                //
                line_number_table_attribute ->
                    AddLineNumber(code_attribute -> CodeLength(),
                                  semantic.lex_stream -> Line(end_expr ->
                                                              LeftToken()));

                EmitBranchIfExpression(end_expr, true,
                                       empty ? continue_label : begin_label,
                                       for_statement -> statement);
            }
            else EmitBranch(OP_GOTO, empty ? continue_label : begin_label,
                            for_statement -> statement);
            CompleteLabel(continue_label);
            CompleteLabel(begin_label);

            // Restore pre-loop locals to reset for-init variables.
            // This ensures break targets after the loop don't include for-init variables.
            if (stack_map_generator && saved_pre_loop_locals)
            {
                stack_map_generator->RestoreLocals(saved_pre_loop_locals);
                delete saved_pre_loop_locals;
            }

            delete saved_loop_locals;
            return abrupt && ! for_statement -> can_complete_normally;
        }
    case Ast::FOREACH: // JSR 201
        EmitForeachStatement((AstForeachStatement*) statement);
        return false;
    case Ast::BREAK: // JLS 14.13
        {
            unsigned nesting_level =
                statement -> BreakStatementCast() -> nesting_level;
            AstBlock* over = method_stack -> Block(nesting_level);
            u2 jump_size = (over -> RightToken() - over -> LeftToken() <
                            TOKEN_WIDTH_REQUIRING_GOTOW) ? 3 : 5;
            if (ProcessAbruptExit(nesting_level, jump_size))
            {
                EmitBranch(OP_GOTO, method_stack -> BreakLabel(nesting_level),
                           over);
            }
            return true;
        }
    case Ast::CONTINUE: // JLS 14.14
        {
            unsigned nesting_level =
                statement -> ContinueStatementCast() -> nesting_level;
            AstBlock* over = method_stack -> Block(nesting_level);
            u2 jump_size = (over -> RightToken() - over -> LeftToken() <
                            TOKEN_WIDTH_REQUIRING_GOTOW) ? 3 : 5;
            if (ProcessAbruptExit(nesting_level, jump_size))
            {
                EmitBranch(OP_GOTO,
                           method_stack -> ContinueLabel(nesting_level),
                           over);
            }
            return true;
        }
    case Ast::RETURN: // JLS 14.15
        EmitReturnStatement(statement -> ReturnStatementCast());
        return true;
    case Ast::SUPER_CALL:
        EmitSuperInvocation((AstSuperCall*) statement);
        return false;
    case Ast::THIS_CALL:
        EmitThisInvocation((AstThisCall*) statement);
        return false;
    case Ast::THROW: // JLS 14.16
        EmitExpression(statement -> ThrowStatementCast() -> expression);
        PutOp(OP_ATHROW);
        return true;
    case Ast::SYNCHRONIZED_STATEMENT: // JLS 14.17
        return EmitSynchronizedStatement((AstSynchronizedStatement*) statement);
    case Ast::TRY: // JLS 14.18
        EmitTryStatement((AstTryStatement*) statement);
        return ! statement -> can_complete_normally;
    case Ast::CATCH:   // JLS 14.18
    case Ast::FINALLY: // JLS 14.18
        // handled by TryStatement
        assert(false && "should not get here");
        return false;
    case Ast::ASSERT: // JDK 1.4 (JSR 41)
        EmitAssertStatement((AstAssertStatement*) statement);
        return false;
    case Ast::LOCAL_CLASS: // Class Declaration
        //
        // This is factored out by the front end; and so must be
        // skipped here (remember, interfaces cannot be declared locally).
        //
        return false;
    case Ast::VARIABLE_DECLARATOR:
        //
        // This is not really a statement, but we treat it as one to make
        // initializer blocks easier to intermix with variable declarations.
        //
        InitializeVariable((AstVariableDeclarator*) statement);
        return false;
    default:
        assert(false && "unknown statement kind");
        return false;
    }
}


void ByteCode::EmitReturnStatement(AstReturnStatement* statement)
{
    AstExpression* expression = statement -> expression_opt;

    if (! expression)
    {
        if (ProcessAbruptExit(method_stack -> NestingLevel(0), 1))
            PutOp(OP_RETURN);
    }
    else
    {
        TypeSymbol* type = expression -> Type();
        assert(type != control.void_type);

        EmitExpression(expression);

        if (ProcessAbruptExit(method_stack -> NestingLevel(0), 1, type))
            GenerateReturn(type);
    }
}


bool ByteCode::EmitBlockStatement(AstBlock* block)
{
    assert(stack_depth == 0); // stack empty at start of statement

    method_stack -> Push(block);
    bool abrupt = false;
    for (unsigned i = 0; i < block -> NumStatements() && ! abrupt; i++)
        abrupt = EmitStatement(block -> Statement(i));

    //
    // If contained break statements jump out of this block, define the label.
    //
    if (IsLabelUsed(method_stack -> TopBreakLabel()))
    {
        DefineLabel(method_stack -> TopBreakLabel());
        CompleteLabel(method_stack -> TopBreakLabel());
        abrupt = false;
    }

    if (control.option.g & JopaOption::VARS)
    {
        for (unsigned i = 0; i < block -> NumLocallyDefinedVariables(); i++)
        {
            VariableSymbol* variable = block -> LocallyDefinedVariable(i);
            if (method_stack -> StartPc(variable) == 0xFFFF) // never used
                continue;
#ifdef DUMP
            Coutput << "(56) The symbol \"" << variable -> Name()
                    << "\" numbered " << variable -> LocalVariableIndex()
                    << " was released" << endl;
#endif // DUMP
            local_variable_table_attribute ->
                AddLocalVariable(method_stack -> StartPc(variable),
                                 code_attribute -> CodeLength(),
                                 RegisterName(variable -> ExternalIdentity()),
                                 RegisterUtf8(variable -> Type() -> signature),
                                 variable -> LocalVariableIndex());
        }
    }

    method_stack -> Pop();
    return abrupt;
}


void ByteCode::EmitStatementExpression(AstExpression* expression)
{
    switch (expression -> kind)
    {
    case Ast::CALL:
        EmitMethodInvocation((AstMethodInvocation*) expression, false);
        break;
    case Ast::POST_UNARY:
        EmitPostUnaryExpression((AstPostUnaryExpression*) expression, false);
        break;
    case Ast::PRE_UNARY:
        EmitPreUnaryExpression((AstPreUnaryExpression*) expression, false);
        break;
    case Ast::ASSIGNMENT:
        EmitAssignmentExpression((AstAssignmentExpression*) expression, false);
        break;
    case Ast::CLASS_CREATION:
        EmitClassCreationExpression((AstClassCreationExpression*) expression,
                                    false);
        break;
    default:
        assert(false && "invalid statement expression kind");
    }
}


//
// Generate code for switch statement. Good code generation requires
// detailed knowledge of the target machine. Lacking this, we simply
// choose between LOOKUPSWITCH and TABLESWITCH by picking that
// opcode that takes the least number of bytes in the byte code.
//
// With TABLESWITCH, a target must be provided for every entry in the range
// low..high, even though the user may not have provided an explicit entry,
// in which case the default action is to be taken. For example
// switch (e) {
//  case 1:2:3: act1; break;
//  case 5:6:   act2; break;
//  default: defact; break;
// }
// translates as
// switch (e) {
//  case 1:2:3: act1; break;
//  case 4: goto defa:
//  case 5:6:   act2; break;
//  defa:
//  default: defact;
// }
//
bool ByteCode::EmitSwitchStatement(AstSwitchStatement* switch_statement)
{
    AstBlock* switch_block = switch_statement -> switch_block;
    u2 op_start = code_attribute -> CodeLength();
    unsigned i;
    bool abrupt;

    assert(stack_depth == 0); // stack empty at start of statement

    //
    // String switches require special handling
    //
    if (switch_statement -> is_string_switch)
    {
        return EmitStringSwitchStatement(switch_statement);
    }

    //
    // Optimization: When switching on a constant, emit only those blocks
    // that it will flow through.
    // switch (constant) { ... } => single code path
    //
    if (switch_statement -> expression -> IsConstant())
    {
        CaseElement* target = switch_statement ->
            CaseForValue(DYNAMIC_CAST<IntLiteralValue*>
                         (switch_statement -> expression -> value) -> value);
        if (! target)
            return false;
        //
        // Bring all previously-declared variables into scope, then compile
        // until we run out of blocks or else complete abruptly.
        //
        method_stack -> Push(switch_block);
        for (i = 0; i < target -> block_index; i++)
            EmitSwitchBlockStatement(switch_statement -> Block(i), true);
        abrupt = false;
        for ( ; ! abrupt && i < switch_statement -> NumBlocks(); i++)
        {
            abrupt =
                EmitSwitchBlockStatement(switch_statement -> Block(i), abrupt);
        }

        CloseSwitchLocalVariables(switch_block, op_start);
        if (IsLabelUsed(method_stack -> TopBreakLabel()))
        {
            abrupt = false;
            DefineLabel(method_stack -> TopBreakLabel());
            CompleteLabel(method_stack -> TopBreakLabel());
        }
        method_stack -> Pop();
        return abrupt;
    }

    //
    // Optimization: When there are zero blocks, emit the expression.
    // switch (expr) {} => expr;
    //
    if (! switch_statement -> NumBlocks())
    {
        EmitExpression(switch_statement -> expression, false);
        return false;
    }

    //
    // Optimization: When there is one block labeled by default, emit it.
    // switch (expr) { default: block; } => expr, block
    // switch (expr) { case a: default: block; } => expr, block
    //
    if (switch_statement -> NumBlocks() == 1 &&
        switch_statement -> DefaultCase())
    {
        EmitExpression(switch_statement -> expression, false);
        method_stack -> Push(switch_block);
        abrupt = EmitSwitchBlockStatement(switch_statement -> Block(0), false);
        CloseSwitchLocalVariables(switch_block, op_start);
        if (IsLabelUsed(method_stack -> TopBreakLabel()))
        {
            abrupt = false;
            DefineLabel(method_stack -> TopBreakLabel());
            CompleteLabel(method_stack -> TopBreakLabel());
        }
        method_stack -> Pop();
        return abrupt;
    }

    //
    // Optimization: If there is one non-default label, turn this into an
    // if statement.
    //
    if (switch_statement -> NumCases() == 1)
    {
        //
        // switch (expr) { case a: block; } => if (expr == a) block;
        //
        if (! switch_statement -> DefaultCase())
        {
            EmitExpression(switch_statement -> expression);
            // For enum switch, call ordinal() to convert to int
            if (switch_statement -> is_enum_switch)
            {
                PutOp(OP_INVOKEVIRTUAL);
                PutU2(RegisterLibraryMethodref(control.Enum_ordinalMethod()));
                ChangeStack(1); // Return value (int)
            }
            Label lab;
            if (switch_statement -> Case(0) -> value)
            {
                LoadImmediateInteger(switch_statement -> Case(0) -> value);
                EmitBranch(OP_IF_ICMPNE, lab, switch_block);
            }
            else EmitBranch(OP_IFNE, lab, switch_block);
            method_stack -> Push(switch_block);
            EmitSwitchBlockStatement(switch_statement -> Block(0), false);
            CloseSwitchLocalVariables(switch_block, op_start);
            if (IsLabelUsed(method_stack -> TopBreakLabel()))
            {
                DefineLabel(method_stack -> TopBreakLabel());
                CompleteLabel(method_stack -> TopBreakLabel());
            }
            method_stack -> Pop();
            DefineLabel(lab);
            CompleteLabel(lab);
            return false;
        }
        //
        // TODO: Implement these optimizations.
        // switch (expr) { case a: fallthrough_block; default: block; }
        //  => if (expr == a) fallthrough_block; block;
        // switch (expr) { case a: abrupt_block; default: block; }
        //  => if (expr == a) abrupt_block; else block;
        // switch (expr) { default: fallthrough_block; case a: block; }
        //  => if (expr != a) fallthrough_block; block;
        // switch (expr) { default: abrupt_block; case a: block; }
        //  => if (expr != a) abrupt_block; else block;
        //
    }

    //
    // Use tableswitch if size of tableswitch case is no more than 32 bytes
    // (8 words) more code than lookup case.
    //
    bool use_lookup = true; // set if using LOOKUPSWITCH opcode
    unsigned ncases = switch_statement -> NumCases();
    unsigned nlabels = ncases;
    i4 high = 0,
       low = 0;
    if (ncases)
    {
        low = switch_statement -> Case(0) -> value;
        high = switch_statement -> Case(ncases - 1) -> value;
        assert(low <= high);

        //
        // Workaround for Sun JVM TABLESWITCH bug in JDK 1.2, 1.3
        // when case values of 0x7ffffff0 through 0x7fffffff are used.
        // Force the generation of a LOOKUPSWITCH in these circumstances.
        //
        if (high < 0x7ffffff0L ||
            control.option.target >= JopaOption::SDK1_4)
        {
            // We want to compute (1 + (high - low + 1)) < (ncases * 2 + 8).
            // However, we must beware of integer overflow.
            i4 range = high - low + 1;
            if (range > 0 && (unsigned) range < (ncases * 2 + 8))
            {
                use_lookup = false; // use tableswitch
                nlabels = range;
                assert(nlabels >= ncases);
            }
        }
    }

    // Enum switches must use lookupswitch because enum case labels don't have
    // an IntLiteralValue - they use map_index from the sorted case list.
    if (switch_statement -> is_enum_switch)
        use_lookup = true;

    //
    // Set up the environment for the switch block.  This must be done before
    // emitting the expression, in case the expression is an assignment.
    //
    method_stack -> Push(switch_block);

    //
    // Reset the line number before evaluating the expression
    //
    line_number_table_attribute ->
        AddLineNumber(code_attribute -> CodeLength(),
                      semantic.lex_stream -> Line(switch_statement ->
                                                  expression -> LeftToken()));
    EmitExpression(switch_statement -> expression);

    // For enum switch, call ordinal() to convert to int
    if (switch_statement -> is_enum_switch)
    {
        PutOp(OP_INVOKEVIRTUAL);
        PutU2(RegisterLibraryMethodref(control.Enum_ordinalMethod()));
        ChangeStack(1); // Return value (int)
    }

    PutOp(use_lookup ? OP_LOOKUPSWITCH : OP_TABLESWITCH);
    op_start = last_op_pc; // pc at start of instruction

    //
    // Supply any needed padding.
    //
    while (code_attribute -> CodeLength() % 4 != 0)
        PutU1(0);

    //
    // Note that if there is no default clause in switch statement, we create
    // one that corresponds to do nothing and branches to start of next
    // statement. The default label is case_labels[nlabels].
    //
    Label* case_labels = new Label[nlabels + 1];
    UseLabel(case_labels[nlabels], 4,
             code_attribute -> CodeLength() - op_start);

    if (use_lookup)
    {
        PutU4(ncases);
        for (i = 0; i < ncases; i++)
        {
            PutU4(static_cast<u4>(switch_statement -> Case(i) -> value));
            UseLabel(case_labels[i], 4,
                     code_attribute -> CodeLength() - op_start);
        }
    }
    else
    {
        PutU4(static_cast<u4>(low));
        PutU4(static_cast<u4>(high));
        for (i = 0; i < nlabels; i++)
        {
            UseLabel(case_labels[i], 4,
                     code_attribute -> CodeLength() - op_start);
        }
    }

    //
    // March through switch block statements, compiling blocks in proper
    // order. We must respect order in which blocks are seen so that blocks
    // lacking a terminal break fall through to the proper place.
    //
    abrupt = false;
    for (i = 0; i < switch_block -> NumStatements(); i++)
    {
        AstSwitchBlockStatement* switch_block_statement =
            switch_statement -> Block(i);
        for (unsigned li = 0;
             li < switch_block_statement -> NumSwitchLabels(); li++)
        {
            AstSwitchLabel* switch_label =
                switch_block_statement -> SwitchLabel(li);
            if (use_lookup)
                DefineLabel(case_labels[switch_label -> map_index]);
            else if (switch_label -> expression_opt)
            {
                i4 value = DYNAMIC_CAST<IntLiteralValue*>
                    (switch_label -> expression_opt -> value) -> value;
                DefineLabel(case_labels[value - low]);
            }
            else
            {
                DefineLabel(case_labels[nlabels]);
                //
                // We must also point all inserted cases to the default.
                //
                unsigned j = 1;
                i4 k = low + 1;
                for ( ; j < switch_statement -> NumCases(); j++, k++)
                    while (k != switch_statement -> Case(j) -> value)
                        DefineLabel(case_labels[k++ - low]);
            }
        }
        abrupt = EmitSwitchBlockStatement(switch_block_statement, false);
    }

    CloseSwitchLocalVariables(switch_block, op_start);
    for (i = 0; i <= nlabels; i++)
    {
        if (! case_labels[i].defined)
        {
            abrupt = false;
            DefineLabel(case_labels[i]);
        }
        CompleteLabel(case_labels[i]);
    }
    //
    // If this switch statement was "broken", we define the break label here.
    //
    if (IsLabelUsed(method_stack -> TopBreakLabel()))
    {
        // need define only if used
        DefineLabel(method_stack -> TopBreakLabel());
        CompleteLabel(method_stack -> TopBreakLabel());
        abrupt = false;
    }

    delete [] case_labels;
    method_stack -> Pop();
    assert(abrupt || switch_statement -> can_complete_normally);
    return abrupt;
}


bool ByteCode::EmitStringSwitchStatement(AstSwitchStatement* switch_statement)
{
    AstBlock* switch_block = switch_statement -> switch_block;
    u2 op_start = code_attribute -> CodeLength();
    unsigned i;
    bool abrupt;

    assert(stack_depth == 0);

    if (! switch_statement -> NumBlocks())
    {
        EmitExpression(switch_statement -> expression, false);
        return false;
    }

    method_stack -> Push(switch_block);

    line_number_table_attribute ->
        AddLineNumber(code_attribute -> CodeLength(),
                      semantic.lex_stream -> Line(switch_statement ->
                                                  expression -> LeftToken()));

    EmitExpression(switch_statement -> expression);

    u2 string_var = switch_block -> block_symbol -> helper_variable_index;

    // Track local variable for StackMapTable
    if (stack_map_generator)
    {
        stack_map_generator->SetLocal(string_var, control.String());
    }

    if (string_var <= 3)
        PutOp((Opcode) (OP_ASTORE_0 + string_var));
    else
        PutOpWide(OP_ASTORE, string_var);

    Label default_jump_label;

    if (string_var <= 3)
        PutOp((Opcode) (OP_ALOAD_0 + string_var));
    else
        PutOpWide(OP_ALOAD, string_var);
    EmitBranch(OP_IFNULL, default_jump_label, switch_block);

    if (string_var <= 3)
        PutOp((Opcode) (OP_ALOAD_0 + string_var));
    else
        PutOpWide(OP_ALOAD, string_var);
    PutOp(OP_INVOKEVIRTUAL);
    PutU2(RegisterLibraryMethodref(control.String_hashCodeMethod()));
    ChangeStack(1);

    unsigned ncases = switch_statement -> NumCases();
    unsigned nlabels = ncases;
    i4 high = 0, low = 0;
    bool use_lookup = true;

    if (ncases)
    {
        low = switch_statement -> Case(0) -> value;
        high = switch_statement -> Case(ncases - 1) -> value;
        i4 range = high - low + 1;
        if (range > 0 && (unsigned) range < (ncases * 2 + 8))
        {
            use_lookup = false;
            nlabels = range;
        }
    }

    PutOp(use_lookup ? OP_LOOKUPSWITCH : OP_TABLESWITCH);
    u2 switch_op_start = last_op_pc;

    while (code_attribute -> CodeLength() % 4 != 0)
        PutU1(0);

    Label* hash_labels = new Label[nlabels + 1];
    UseLabel(hash_labels[nlabels], 4,
             code_attribute -> CodeLength() - switch_op_start);

    if (use_lookup)
    {
        PutU4(ncases);
        for (i = 0; i < ncases; i++)
        {
            PutU4(static_cast<u4>(switch_statement -> Case(i) -> value));
            UseLabel(hash_labels[i], 4,
                     code_attribute -> CodeLength() - switch_op_start);
        }
    }
    else
    {
        PutU4(static_cast<u4>(low));
        PutU4(static_cast<u4>(high));
        unsigned j = 0;
        for (i4 k = low; k <= high; k++, j++)
        {
            while (j < ncases && switch_statement -> Case(j) -> value < k)
                j++;
            if (j < ncases && switch_statement -> Case(j) -> value == k)
            {
                UseLabel(hash_labels[j], 4,
                         code_attribute -> CodeLength() - switch_op_start);
            }
            else
            {
                UseLabel(hash_labels[nlabels], 4,
                         code_attribute -> CodeLength() - switch_op_start);
            }
        }
    }

    Label* case_labels = new Label[switch_statement -> NumCases() + 1];

    for (i = 0; i < ncases; i++)
    {
        DefineLabel(hash_labels[i]);
        CompleteLabel(hash_labels[i]);

        unsigned j = i;
        while (j < ncases && switch_statement -> Case(j) -> value ==
               switch_statement -> Case(i) -> value)
        {
            CaseElement* case_elt = switch_statement -> Case(j);
            AstSwitchLabel* label = switch_statement ->
                Block(case_elt -> block_index) ->
                SwitchLabel(case_elt -> case_index);

            if (string_var <= 3)
                PutOp((Opcode) (OP_ALOAD_0 + string_var));
            else
                PutOpWide(OP_ALOAD, string_var);

            Utf8LiteralValue* str_value = DYNAMIC_CAST<Utf8LiteralValue*>
                (label -> expression_opt -> value);
            PutOp(OP_LDC_W);
            PutU2(RegisterString(str_value));
            // Note: LDC_W already adds +1 to stack via PutOp

            PutOp(OP_INVOKEVIRTUAL);
            PutU2(RegisterLibraryMethodref(control.String_equalsMethod()));
            // equals(Object)Z: default -1 for receiver, 0 adjustment needed
            // (arg -1 + return +1 = 0)

            EmitBranch(OP_IFNE, case_labels[j], switch_block);
            j++;
        }
        i = j - 1;
        EmitBranch(OP_GOTO, hash_labels[nlabels], switch_block);
    }

    // Emit case bodies in order. Define labels at the appropriate positions.
    abrupt = false;
    for (unsigned block_idx = 0; block_idx < switch_block -> NumStatements();
         block_idx++)
    {
        AstSwitchBlockStatement* switch_block_stmt =
            switch_statement -> Block(block_idx);

        for (unsigned label_idx = 0;
             label_idx < switch_block_stmt -> NumSwitchLabels(); label_idx++)
        {
            AstSwitchLabel* switch_label =
                switch_block_stmt -> SwitchLabel(label_idx);
            if (switch_label -> expression_opt)
            {
                // Non-default case: define case label
                DefineLabel(case_labels[switch_label -> map_index]);
                CompleteLabel(case_labels[switch_label -> map_index]);
            }
            else
            {
                // Default case: define hash no-match and null-check labels here
                DefineLabel(hash_labels[nlabels]);
                CompleteLabel(hash_labels[nlabels]);
                DefineLabel(default_jump_label);
                CompleteLabel(default_jump_label);
            }
        }

        abrupt = EmitSwitchBlockStatement(switch_block_stmt, false);
    }

    CloseSwitchLocalVariables(switch_block, op_start);

    // Complete any undefined case labels (and no-match/default if no default case)
    for (i = 0; i < ncases; i++)
    {
        if (! case_labels[i].defined)
        {
            abrupt = false;
            DefineLabel(case_labels[i]);
        }
        CompleteLabel(case_labels[i]);
    }

    // If no default case was defined, define the no-match and null labels here
    if (! hash_labels[nlabels].defined)
    {
        DefineLabel(hash_labels[nlabels]);
        abrupt = false;
    }
    CompleteLabel(hash_labels[nlabels]);

    if (! default_jump_label.defined)
    {
        DefineLabel(default_jump_label);
        abrupt = false;
    }
    CompleteLabel(default_jump_label);

    if (IsLabelUsed(method_stack -> TopBreakLabel()))
    {
        DefineLabel(method_stack -> TopBreakLabel());
        CompleteLabel(method_stack -> TopBreakLabel());
        abrupt = false;
    }

    delete [] hash_labels;
    delete [] case_labels;
    method_stack -> Pop();
    return abrupt;
}


bool ByteCode::EmitSwitchBlockStatement(AstSwitchBlockStatement* block,
                                        bool abrupt)
{
    for (unsigned i = 0; i < block -> NumStatements(); i++)
    {
        if (! abrupt)
            abrupt = EmitStatement(block -> Statement(i));
        else if (block -> Statement(i) -> LocalVariableStatementCast())
        {
            //
            // In a switch statement, local variable declarations are
            // accessible in other case labels even if the declaration
            // itself is unreachable.
            //
            AstLocalVariableStatement* lvs =
                (AstLocalVariableStatement*) block -> Statement(i);
            for (unsigned j = 0; j < lvs -> NumVariableDeclarators(); j++)
            {
                AstVariableDeclarator* declarator =
                    lvs -> VariableDeclarator(j);
                if (control.option.g & JopaOption::VARS)
                {
                    method_stack -> StartPc(declarator -> symbol) =
                        code_attribute -> CodeLength();
                }
                if (declarator -> symbol -> Type() -> num_dimensions > 255)
                {
                    semantic.ReportSemError(SemanticError::ARRAY_OVERFLOW,
                                            declarator);
                }
            }
        }
    }
    return abrupt;
}


void ByteCode::CloseSwitchLocalVariables(AstBlock* switch_block,
                                         u2 op_start)
{
    if (control.option.g & JopaOption::VARS)
    {
        for (unsigned i = 0;
             i < switch_block -> NumLocallyDefinedVariables(); i++)
        {
            VariableSymbol* variable =
                switch_block -> LocallyDefinedVariable(i);
            if (method_stack -> StartPc(variable) > op_start)
            {
                if (method_stack -> StartPc(variable) == 0xFFFF) // never used
                    continue;
#ifdef DUMP
                Coutput << "(58) The symbol \"" << variable -> Name()
                        << "\" numbered " << variable -> LocalVariableIndex()
                        << " was released" << endl;
#endif // DUMP
                local_variable_table_attribute ->
                    AddLocalVariable(method_stack -> StartPc(variable),
                                     code_attribute -> CodeLength(),
                                     RegisterName(variable -> ExternalIdentity()),
                                     RegisterUtf8(variable -> Type() -> signature),
                                     variable -> LocalVariableIndex());
            }
        }
    }
}


//
// Java 7: Helper to emit close() calls for try-with-resources
// Closes resources in reverse declaration order with exception suppression.
//
// Exception handling follows JLS 14.20.3:
// - If primary_exception exists (try body threw): catch close exceptions and
//   call primary_exception.addSuppressed(closeException)
// - If primary_exception is null: accumulate first close exception and
//   suppress subsequent ones with close_exception.addSuppressed()
// - After all closes, throw close_exception if primary_exception is null
//
// Variable layout (relative to variable_index):
//   +0: primary_exception (set by catch-all handler if try throws)
//   +2: close_exception (accumulated close exceptions in normal path)
//
void ByteCode::EmitResourceCleanup(AstTryStatement* statement, int variable_index)
{
    // Close resources in reverse order
    for (int r = statement -> NumResources() - 1; r >= 0; r--)
    {
        AstLocalVariableStatement* resource = statement -> Resource(r);
        for (int v = resource -> NumVariableDeclarators() - 1; v >= 0; v--)
        {
            AstVariableDeclarator* vd = resource -> VariableDeclarator(v);
            VariableSymbol* var = vd -> symbol;
            if (! var) continue;

            // Load the resource variable
            LoadLocal(var -> LocalVariableIndex(), var -> Type());

            // Check for null before calling close()
            Label skip_close;
            Label after_catch;
            PutOp(OP_DUP);
            int stack_at_branch = stack_depth;  // = 2

            EmitBranch(OP_IFNULL, skip_close);
            // Fall-through (not null) path: stack = 1 [ref]

            // Record start of try block for close()
            u2 close_try_start = code_attribute -> CodeLength();

            // Call close() - AutoCloseable is an interface
            PutOp(OP_INVOKEINTERFACE);
            PutU2(RegisterLibraryMethodref(control.AutoCloseable_closeMethod()));
            PutU1(1);  // 1 argument (just 'this')
            PutU1(0);  // reserved
            // After close(): stack = 0

            // Record end of try block
            u2 close_try_end = code_attribute -> CodeLength();

            EmitBranch(OP_GOTO, after_catch, statement);

            // Exception handler for close(): catches any Throwable
            u2 catch_handler_pc = code_attribute -> CodeLength();

            // Record StackMapTable frame for exception handler entry
            if (stack_map_generator)
            {
                stack_map_generator->ClearStack();
                stack_map_generator->PushType(control.Throwable());
                stack_map_generator->RecordFrame(catch_handler_pc);
                stack_map_generator->ClearStack(); // Reset for subsequent non-handler code
            }

            stack_depth = 1;  // caught exception is on stack

            // Logic:
            // if (primary_exception != null):
            //     primary_exception.addSuppressed(caught)
            // else if (close_exception != null):
            //     close_exception.addSuppressed(caught)
            // else:
            //     close_exception = caught

            Label check_close_exception;
            Label first_close_exception;

            // Load primary_exception and check if non-null
            // Stack: [caught]
            LoadLocal(variable_index, control.Throwable());
            // Stack: [caught, primary]
            PutOp(OP_DUP);
            // Stack: [caught, primary, primary]
            EmitBranch(OP_IFNULL, check_close_exception);
            // Stack: [caught, primary]

            // Primary exception exists: call primary.addSuppressed(caught)
            // or just discard if nosuppressed is set
            if (control.option.nosuppressed)
            {
                // Stack: [caught, primary]
                PutOp(OP_POP2);  // discard both
                // Stack: []
            }
            else
            {
                PutOp(OP_SWAP);
                // Stack: [primary, caught]
                PutOp(OP_INVOKEVIRTUAL);
                PutU2(RegisterLibraryMethodref(control.Throwable_addSuppressedMethod()));
                // Stack: []
            }
            EmitBranch(OP_GOTO, after_catch, statement);

            // check_close_exception:
            stack_depth = 2;  // [caught, null]
            // Synchronize stack_map_generator with expected stack state
            if (stack_map_generator)
            {
                stack_map_generator->ClearStack();
                stack_map_generator->PushType(control.Throwable());  // caught
                stack_map_generator->PushType(control.Throwable());  // null (primary)
            }
            DefineLabel(check_close_exception);
            CompleteLabel(check_close_exception);
            PutOp(OP_POP);  // pop null (was primary)
            // Stack: [caught]

            // Load close_exception and check if non-null
            LoadLocal(variable_index + 2, control.Throwable());
            // Stack: [caught, close]
            PutOp(OP_DUP);
            // Stack: [caught, close, close]
            EmitBranch(OP_IFNULL, first_close_exception);
            // Stack: [caught, close]

            // Close exception exists: call close.addSuppressed(caught)
            // or just discard if nosuppressed is set
            if (control.option.nosuppressed)
            {
                // Stack: [caught, close]
                PutOp(OP_POP2);  // discard both
                // Stack: []
            }
            else
            {
                PutOp(OP_SWAP);
                // Stack: [close, caught]
                PutOp(OP_INVOKEVIRTUAL);
                PutU2(RegisterLibraryMethodref(control.Throwable_addSuppressedMethod()));
                // Stack: []
            }
            EmitBranch(OP_GOTO, after_catch, statement);

            // first_close_exception: no previous exception, store caught as close_exception
            stack_depth = 2;  // [caught, null]
            // Synchronize stack_map_generator with expected stack state
            if (stack_map_generator)
            {
                stack_map_generator->ClearStack();
                stack_map_generator->PushType(control.Throwable());  // caught
                stack_map_generator->PushType(control.Throwable());  // null (close)
            }
            DefineLabel(first_close_exception);
            CompleteLabel(first_close_exception);
            PutOp(OP_POP);  // pop null (was close)
            // Stack: [caught]
            StoreLocal(variable_index + 2, control.Throwable());
            // Stack: []
            EmitBranch(OP_GOTO, after_catch, statement);

            // Null path: resource was null, just pop it
            stack_depth = stack_at_branch - 1;  // IFNULL pops 1
            // Synchronize stack_map_generator with expected stack state
            if (stack_map_generator)
            {
                stack_map_generator->ClearStack();
                stack_map_generator->PushType(var->Type());  // one ref left after IFNULL consumes top
            }
            DefineLabel(skip_close);
            CompleteLabel(skip_close);
            PutOp(OP_POP);  // pop the null reference
            // Stack: []

            // After all paths converge
            stack_depth = 0;
            // Synchronize stack_map_generator
            if (stack_map_generator)
            {
                stack_map_generator->ClearStack();
            }
            DefineLabel(after_catch);
            CompleteLabel(after_catch);

            // Register exception handler for close() call
            // Catch type is java/lang/Throwable
            code_attribute -> AddException(close_try_start, close_try_end,
                                           catch_handler_pc,
                                           RegisterClass(control.Throwable()));
        }
    }

    //
    // After all resources are closed, check if we need to throw close_exception.
    // This happens when primary_exception is null (normal path) and close_exception
    // is non-null (some close() threw).
    //
    Label done_cleanup;
    // if (primary_exception != null) goto done_cleanup (it will be rethrown by caller)
    LoadLocal(variable_index, control.Throwable());
    EmitBranch(OP_IFNONNULL, done_cleanup);
    // stack = 0 after IFNONNULL

    // if (close_exception == null) goto done_cleanup
    LoadLocal(variable_index + 2, control.Throwable());
    EmitBranch(OP_IFNULL, done_cleanup);
    // stack = 0 after IFNULL

    // Reload close_exception and throw it
    LoadLocal(variable_index + 2, control.Throwable());
    PutOp(OP_ATHROW);

    // done_cleanup: both paths arrive with stack = 0
    stack_depth = 0;
    // Synchronize stack_map_generator
    if (stack_map_generator)
    {
        stack_map_generator->ClearStack();
    }
    DefineLabel(done_cleanup);
    CompleteLabel(done_cleanup);
}

//
//  13.18       The try statement
//
void ByteCode::EmitTryStatement(AstTryStatement* statement)
{
    //
    // If the finally label in the surrounding block is used by a try
    // statement, it is cleared after the finally block associated with the
    // try statement has been processed.
    //
    assert(method_stack -> TopFinallyLabel().uses.Length() == 0);
    assert(method_stack -> TopFinallyLabel().defined == false);
    assert(method_stack -> TopFinallyLabel().definition == 0);

    //
    // Java 7: Emit resource initializations for try-with-resources
    // Resources are stored in local variables before the try block
    //
    bool has_resources = statement -> NumResources() > 0;
    for (unsigned r = 0; r < statement -> NumResources(); r++)
    {
        AstLocalVariableStatement* resource = statement -> Resource(r);
        for (unsigned v = 0; v < resource -> NumVariableDeclarators(); v++)
        {
            AstVariableDeclarator* vd = resource -> VariableDeclarator(v);
            DeclareLocalVariable(vd);
        }
    }

    //
    // Java 7: Initialize exception variables for try-with-resources suppression.
    // variable_index+0: primary exception (set by catch-all handler if try throws)
    // variable_index+2: close exception (accumulated close exceptions in normal path)
    //
    if (has_resources)
    {
        int variable_index = method_stack -> TopBlock() ->
            block_symbol -> helper_variable_index;
        PutOp(OP_ACONST_NULL);
        StoreLocal(variable_index, control.Throwable());  // primary_exception = null
        PutOp(OP_ACONST_NULL);
        StoreLocal(variable_index + 2, control.Throwable());  // close_exception = null
    }

    u2 start_try_block_pc = code_attribute -> CodeLength(); // start pc
    assert(method_stack -> TopHandlerRangeStart().Length() == 0 &&
           method_stack -> TopHandlerRangeEnd().Length() == 0);
    method_stack -> TopHandlerRangeStart().Push(start_try_block_pc);

    // Save local state at try block start for exception handler frames
    Tuple<StackMapTableAttribute::VerificationTypeInfo>* saved_locals = NULL;
    if (stack_map_generator)
    {
        saved_locals = stack_map_generator->SaveLocals();
    }

    // Java 7: Try-with-resources creates a synthetic finally for close() calls
    bool emit_explicit_finally = statement -> finally_clause_opt &&
        ! IsNop(statement -> finally_clause_opt -> block);
    bool emit_finally_clause = emit_explicit_finally || has_resources;

    //
    // If we determined the finally clause is a nop, remove the tag
    // TRY_CLAUSE_WITH_FINALLY so that abrupt completions do not emit JSR.
    // On the other hand, if the finally clause cannot complete normally,
    // change the tag to ABRUPT_TRY_FINALLY so that abrupt completions emit
    // a GOTO instead of a JSR. Also, mark a try block which has a catch
    // clause but no finally clause, in case an abrupt exit forces a split
    // in the range of protected code.
    //
    if (statement -> finally_clause_opt)
    {
        if (! emit_explicit_finally && ! has_resources)
        {
            statement -> block -> SetTag(AstBlock::NONE);
        }
        else if (! statement -> finally_clause_opt -> block ->
                 can_complete_normally)
        {
            statement -> block -> SetTag(AstBlock::ABRUPT_TRY_FINALLY);
        }
    }
    // For try-with-resources without explicit finally, set the finally tag
    else if (has_resources)
    {
        statement -> block -> SetTag(AstBlock::TRY_CLAUSE_WITH_FINALLY);
    }
    if (statement -> block -> Tag() == AstBlock::NONE &&
        statement -> NumCatchClauses())
    {
        statement -> block -> SetTag(AstBlock::TRY_CLAUSE_WITH_CATCH);
    }
    bool abrupt = EmitBlockStatement(statement -> block);

    //
    // The computation of end_try_block_pc, the instruction following the last
    // instruction in the body of the try block, does not include the code, if
    // any, needed to call a finally block or skip to the end of the try
    // statement.
    //
    u2 end_try_block_pc = code_attribute -> CodeLength();
    Tuple<u2> handler_starts(method_stack -> TopHandlerRangeStart());
    Tuple<u2> handler_ends(method_stack -> TopHandlerRangeEnd());
    handler_ends.Push(end_try_block_pc);
    assert(handler_starts.Length() == handler_ends.Length());

    //
    // If try block is not empty, process catch clauses, including "special"
    // clause for finally.
    //
    if (start_try_block_pc != end_try_block_pc)
    {
        // Use the label in the block immediately enclosing try statement.
        Label& finally_label = method_stack -> TopFinallyLabel();
        Label end_label;

        // Java 7+: Store try statement for inlined finally support in ProcessAbruptExit
        // For Java 7+, we inline finally code instead of using JSR/RET
        bool inline_finally = control.option.target >= JopaOption::SDK1_7 && emit_finally_clause;
        if (inline_finally)
        {
            method_stack -> SetTryStatement(method_stack -> TopNestingLevel(), statement);
        }

        //
        // If try block completes normally, skip code for catch blocks.
        //
        if (! abrupt &&
            (emit_finally_clause || statement -> NumCatchClauses()))
        {
            EmitBranch(OP_GOTO, end_label, statement);
        }

        for (unsigned i = 0; i < statement -> NumCatchClauses(); i++)
        {
            u2 handler_pc = code_attribute -> CodeLength();

            AstCatchClause* catch_clause = statement -> CatchClause(i);
            VariableSymbol* parameter_symbol =
                catch_clause -> parameter_symbol;

            //
            // Record StackMapTable frame for exception handler entry point.
            // At this point, stack has exactly one item: the caught exception.
            // Use saved locals from try block start for consistent frame.
            //
            if (stack_map_generator && saved_locals)
            {
                // Restore locals to try block start state
                stack_map_generator->RestoreLocals(saved_locals);
                // Clear stack and push the exception type
                stack_map_generator->ClearStack();
                stack_map_generator->PushType(parameter_symbol->Type());
                stack_map_generator->RecordFrame(handler_pc);
                stack_map_generator->ClearStack(); // Reset for subsequent non-handler code
            }

            assert(stack_depth == 0);
            stack_depth = 1; // account for the exception already on the stack
            line_number_table_attribute ->
                AddLineNumber(code_attribute -> CodeLength(),
                              semantic.lex_stream -> Line(catch_clause ->
                                                          catch_token));
            //
            // Unless debugging, we don't need to waste a variable on an
            // empty catch.
            //
            if ((control.option.g & JopaOption::VARS) ||
                ! IsNop(catch_clause -> block))
            {
                StoreLocal(parameter_symbol -> LocalVariableIndex(),
                           parameter_symbol -> Type());
            }
            else PutOp(OP_POP);
            u2 handler_type = RegisterClass(parameter_symbol -> Type());
            for (int j = handler_starts.Length(); --j >= 0; )
            {
                code_attribute ->
                    AddException(handler_starts[j], handler_ends[j],
                                 handler_pc, handler_type);
            }

            //
            // If we determined the finally clause is a nop, remove the tag
            // TRY_CLAUSE_WITH_FINALLY so that abrupt completions do not emit
            // JSR. On the other hand, if the finally clause cannot complete
            // normally, change the tag to ABRUPT_TRY_FINALLY so that abrupt
            // completions emit a GOTO instead of a JSR.
            //
            if (statement -> finally_clause_opt)
            {
                if (! emit_finally_clause)
                    catch_clause -> block -> SetTag(AstBlock::NONE);
                else if (! statement -> finally_clause_opt -> block ->
                         can_complete_normally)
                {
                    catch_clause -> block ->
                        SetTag(AstBlock::ABRUPT_TRY_FINALLY);
                }
            }
            abrupt = EmitBlockStatement(catch_clause -> block);

            if (control.option.g & JopaOption::VARS)
            {
                local_variable_table_attribute ->
                    AddLocalVariable(handler_pc,
                                     code_attribute -> CodeLength(),
                                     RegisterName(parameter_symbol -> ExternalIdentity()),
                                     RegisterUtf8(parameter_symbol -> Type() -> signature),
                                     parameter_symbol -> LocalVariableIndex());
            }

            //
            // Reset the catch variable in StackMapGenerator after catch block scope ends.
            // This prevents the catch variable from polluting frames at merge points.
            //
            if (stack_map_generator)
            {
                stack_map_generator->SetLocal(parameter_symbol->LocalVariableIndex(),
                    StackMapTableAttribute::VerificationTypeInfo(
                        StackMapTableAttribute::VerificationTypeInfo::TYPE_Top));
            }

            //
            // If catch block completes normally, skip further catch blocks.
            //
            if (! abrupt && (emit_finally_clause ||
                             i < (statement -> NumCatchClauses() - 1)))
            {
                EmitBranch(OP_GOTO, end_label, statement);
            }
        }
        //
        // If this try statement contains a finally clause (explicit or synthetic), then ...
        //
        if (emit_finally_clause)
        {
            int variable_index = method_stack -> TopBlock() ->
                block_symbol -> helper_variable_index;
            u2 finally_start_pc = code_attribute -> CodeLength();
            u2 special_end_pc = finally_start_pc;

            // Record StackMapTable frame for finally exception handler entry
            // Use saved locals from try block start for consistent frame
            if (stack_map_generator && saved_locals)
            {
                stack_map_generator->RestoreLocals(saved_locals);
                stack_map_generator->ClearStack();
                stack_map_generator->PushType(control.Throwable());
                stack_map_generator->RecordFrame(finally_start_pc);
                stack_map_generator->ClearStack(); // Reset for subsequent non-handler code
            }

            // For try-with-resources, the synthetic finally can always complete normally
            // (it just calls close() on resources)
            bool finally_can_complete = has_resources ||
                (emit_explicit_finally && statement -> finally_clause_opt -> block ->
                 can_complete_normally);

            //
            // Emit code for "special" handler to make sure finally clause is
            // invoked in case an otherwise uncaught exception is thrown in the
            // try block, or an exception is thrown from within a catch block.
            //
            assert(stack_depth == 0);
            stack_depth = 1; // account for the exception already on stack

            if (inline_finally)
            {
                // Java 7+: Inline the finally code instead of using JSR/RET
                if (finally_can_complete)
                {
                    StoreLocal(variable_index, control.Throwable()); // Save exception
                    // Emit inlined finally code
                    if (has_resources)
                    {
                        EmitResourceCleanup(statement, variable_index);
                    }
                    if (emit_explicit_finally)
                    {
                        EmitBlockStatement(statement -> finally_clause_opt -> block);
                    }
                    special_end_pc = code_attribute -> CodeLength();
                    LoadLocal(variable_index, control.Throwable()); // reload, and
                    PutOp(OP_ATHROW); // rethrow exception
                }
                else
                {
                    // Finally cannot complete normally, just pop exception
                    PutOp(OP_POP);
                    if (has_resources)
                    {
                        EmitResourceCleanup(statement, variable_index);
                    }
                    if (emit_explicit_finally)
                    {
                        EmitBlockStatement(statement -> finally_clause_opt -> block);
                    }
                }

                method_stack -> TopHandlerRangeEnd().Push(special_end_pc);
                unsigned count = method_stack -> TopHandlerRangeStart().Length();
                assert(count == method_stack -> TopHandlerRangeEnd().Length());
                while (count--)
                {
                    code_attribute ->
                        AddException(method_stack -> TopHandlerRangeStart().Pop(),
                                     method_stack -> TopHandlerRangeEnd().Pop(),
                                     finally_start_pc, 0);
                }

                // For normal completion path, emit inlined finally code
                if (IsLabelUsed(end_label))
                {
                    // Reset stack_map_generator's local state for normal path
                    // The exception handler set variable_index to Throwable,
                    // but the normal path (GOTO from try body) doesn't have it set
                    if (stack_map_generator)
                    {
                        // Reset exception-handler-specific locals to Top
                        stack_map_generator->SetLocal(variable_index,
                            StackMapTableAttribute::VerificationTypeInfo(
                                StackMapTableAttribute::VerificationTypeInfo::TYPE_Top));
                        // Also reset the return address slot if it was used
                        stack_map_generator->SetLocal(variable_index + 1,
                            StackMapTableAttribute::VerificationTypeInfo(
                                StackMapTableAttribute::VerificationTypeInfo::TYPE_Top));
                        if (has_resources)
                        {
                            // Reset close_exception slot too
                            stack_map_generator->SetLocal(variable_index + 2,
                                StackMapTableAttribute::VerificationTypeInfo(
                                    StackMapTableAttribute::VerificationTypeInfo::TYPE_Top));
                        }
                        stack_map_generator->ClearStack();
                    }
                    DefineLabel(end_label);
                    CompleteLabel(end_label);
                    if (has_resources)
                    {
                        EmitResourceCleanup(statement, variable_index);
                    }
                    if (emit_explicit_finally)
                    {
                        EmitBlockStatement(statement -> finally_clause_opt -> block);
                    }
                }
                // Don't use finally_label for Java 7+
            }
            else
            {
                // Java 6 and earlier: Use JSR/RET for finally
                if (finally_can_complete)
                {
                    StoreLocal(variable_index, control.Throwable()); // Save,
                    EmitBranch(OP_JSR, finally_label, statement);
                    special_end_pc = code_attribute -> CodeLength();
                    LoadLocal(variable_index, control.Throwable()); // reload, and
                    PutOp(OP_ATHROW); // rethrow exception.
                }
                else
                {
                    //
                    // Ignore the exception already on the stack, since we know
                    // the finally clause overrides it.
                    //
                    PutOp(OP_POP);
                }
                method_stack -> TopHandlerRangeEnd().Push(special_end_pc);
                unsigned count = method_stack -> TopHandlerRangeStart().Length();
                assert(count == method_stack -> TopHandlerRangeEnd().Length());
                while (count--)
                {
                    code_attribute ->
                        AddException(method_stack -> TopHandlerRangeStart().Pop(),
                                     method_stack -> TopHandlerRangeEnd().Pop(),
                                     finally_start_pc, 0);
                }

                //
                // Generate code for finally clause. If the finally block can
                // complete normally, this is reached from a JSR, so save the
                // return address. Otherwise, this is reached from a GOTO.
                //
                DefineLabel(finally_label);
                assert(stack_depth == 0);
                if (finally_can_complete)
                {
                    stack_depth = 1; // account for the return location on stack
                    StoreLocal(variable_index + 1, control.Object());
                }
                else if (IsLabelUsed(end_label))
                {
                    DefineLabel(end_label);
                    CompleteLabel(end_label);
                }

                // Java 7: Emit close() calls for resources before explicit finally
                if (has_resources)
                {
                    EmitResourceCleanup(statement, variable_index);
                }

                // Emit explicit finally block if present
                if (emit_explicit_finally)
                {
                    EmitBlockStatement(statement -> finally_clause_opt -> block);
                }

                //
                // If a finally block can complete normally, return to the saved
                // address of the caller.
                //
                if (finally_can_complete)
                {
                    PutOpWide(OP_RET, variable_index + 1);
                    //
                    // Now, if the try or catch blocks complete normally, execute
                    // the finally block before advancing to next statement. We
                    // need to trap one more possibility of an asynchronous
                    // exception before the jsr has started.
                    //
                    if (IsLabelUsed(end_label))
                    {
                        DefineLabel(end_label);
                        CompleteLabel(end_label);
                        EmitBranch(OP_JSR, finally_label, statement);
                        special_end_pc = code_attribute -> CodeLength();
                        code_attribute -> AddException(special_end_pc - 3,
                                                       special_end_pc,
                                                       finally_start_pc, 0);
                    }
                }
                CompleteLabel(finally_label);
            }
        }
        else
        {
            //
            // Finally block is not present, advance to next statement, and
            // clean up the handler start/end ranges.
            //
            assert(! IsLabelUsed(finally_label));
            DefineLabel(end_label);
            CompleteLabel(end_label);
            method_stack -> TopHandlerRangeStart().Reset();
            method_stack -> TopHandlerRangeEnd().Reset();
        }
    }
    else
    {
        //
        // Try block was empty; skip all catch blocks, and a finally block
        // is treated normally. For try-with-resources, just close resources.
        //
        method_stack -> TopHandlerRangeStart().Reset();
        if (has_resources)
        {
            int variable_index = method_stack -> TopBlock() ->
                block_symbol -> helper_variable_index;
            EmitResourceCleanup(statement, variable_index);
        }
        if (emit_explicit_finally)
            EmitBlockStatement(statement -> finally_clause_opt -> block);
    }

    // Clean up saved locals
    if (saved_locals)
        delete saved_locals;
}


//
// Exit to block at level, freeing monitor locks and invoking finally
// clauses as appropriate. The width is 1 for return, 3 for normal a normal
// GOTO (from a break or continue), or 5 for a GOTO_W. The return is true
// unless some intervening finally block cannot complete normally.
//
bool ByteCode::ProcessAbruptExit(unsigned level, u2 width,
                                 TypeSymbol* return_type)
{
    int variable_index = -1;
    //
    // We must store the return value in a variable, rather than on the
    // stack, in case a finally block contains an embedded try-catch which
    // wipes out the stack.
    //
    if (return_type)
    {
        for (unsigned i = method_stack -> Size() - 1;
             i > 0 && method_stack -> NestingLevel(i) != level; i--)
        {
            unsigned nesting_level = method_stack -> NestingLevel(i);
            unsigned enclosing_level = method_stack -> NestingLevel(i - 1);
            AstBlock* block = method_stack -> Block(nesting_level);
            if (block -> Tag() == AstBlock::TRY_CLAUSE_WITH_FINALLY)
            {
                variable_index = method_stack -> Block(enclosing_level) ->
                    block_symbol -> helper_variable_index + 2;
            }
            else if (block -> Tag() == AstBlock::ABRUPT_TRY_FINALLY)
            {
                variable_index = -1;
                PutOp(control.IsDoubleWordType(return_type) ? OP_POP2 : OP_POP);
                break;
            }
        }
    }
    if (variable_index >= 0)
        StoreLocal(variable_index, return_type);

    for (unsigned i = method_stack -> Size() - 1;
         i > 0 && method_stack -> NestingLevel(i) != level; i--)
    {
        unsigned nesting_level = method_stack -> NestingLevel(i);
        unsigned enclosing_level = method_stack -> NestingLevel(i - 1);
        AstBlock* block = method_stack -> Block(nesting_level);
        if (block -> Tag() == AstBlock::TRY_CLAUSE_WITH_FINALLY)
        {
            // Check if we should inline finally for Java 7+
            AstTryStatement* try_stmt = method_stack -> TryStatement(enclosing_level);
            if (try_stmt && control.option.target >= JopaOption::SDK1_7)
            {
                // Java 7+: Inline the finally code instead of using JSR
                int var_index = method_stack -> Block(enclosing_level) ->
                    block_symbol -> helper_variable_index;
                bool has_resources = try_stmt -> NumResources() > 0;
                bool emit_explicit_finally = try_stmt -> finally_clause_opt &&
                    ! IsNop(try_stmt -> finally_clause_opt -> block);
                if (has_resources)
                {
                    EmitResourceCleanup(try_stmt, var_index);
                }
                if (emit_explicit_finally)
                {
                    EmitBlockStatement(try_stmt -> finally_clause_opt -> block);
                }
            }
            else
            {
                // Java 6 and earlier: Use JSR
                EmitBranch(OP_JSR, method_stack -> FinallyLabel(enclosing_level),
                           method_stack -> Block(enclosing_level));
            }
            method_stack -> HandlerRangeEnd(enclosing_level).
                Push(code_attribute -> CodeLength());
        }
        else if (block -> Tag() == AstBlock::ABRUPT_TRY_FINALLY)
        {
            //
            // Ignore the width of the abrupt instruction, because the abrupt
            // finally preempts it.
            //
            width = 0;
            // Check if we should inline finally for Java 7+
            AstTryStatement* try_stmt = method_stack -> TryStatement(enclosing_level);
            if (try_stmt && control.option.target >= JopaOption::SDK1_7)
            {
                // Java 7+: Inline the finally code instead of using GOTO to finally_label
                int var_index = method_stack -> Block(enclosing_level) ->
                    block_symbol -> helper_variable_index;
                bool has_resources = try_stmt -> NumResources() > 0;
                bool emit_explicit_finally = try_stmt -> finally_clause_opt &&
                    ! IsNop(try_stmt -> finally_clause_opt -> block);
                if (has_resources)
                {
                    EmitResourceCleanup(try_stmt, var_index);
                }
                if (emit_explicit_finally)
                {
                    EmitBlockStatement(try_stmt -> finally_clause_opt -> block);
                }
            }
            else
            {
                // Java 6 and earlier: Use GOTO
                EmitBranch(OP_GOTO, method_stack -> FinallyLabel(enclosing_level),
                           method_stack -> Block(enclosing_level));
            }
            method_stack -> HandlerRangeEnd(enclosing_level).
                Push(code_attribute -> CodeLength());
            break;
        }
        else if (block -> Tag() == AstBlock::SYNCHRONIZED)
        {
            //
            // This code must be safe for asynchronous exceptions.  Note that
            // we are splitting the range of instructions covered by the
            // synchronized statement catchall handler.
            //
            int variable_index = method_stack -> Block(enclosing_level) ->
                block_symbol -> helper_variable_index;
            LoadLocal(variable_index, control.Object());
            PutOp(OP_MONITOREXIT);
            method_stack -> HandlerRangeEnd(enclosing_level).
                Push(code_attribute -> CodeLength());
        }
        else if (block -> Tag() == AstBlock::TRY_CLAUSE_WITH_CATCH)
        {
            method_stack -> HandlerRangeEnd(enclosing_level).
                Push(code_attribute -> CodeLength());
        }
    }

    if (variable_index >= 0)
        LoadLocal(variable_index, return_type);
    for (unsigned j = method_stack -> Size() - 1;
         j > 0 && method_stack -> NestingLevel(j) != level; j--)
    {
        unsigned nesting_level = method_stack -> NestingLevel(j);
        unsigned enclosing_level = method_stack -> NestingLevel(j - 1);
        AstBlock* block = method_stack -> Block(nesting_level);
        if (block -> Tag() == AstBlock::SYNCHRONIZED ||
            block -> Tag() == AstBlock::TRY_CLAUSE_WITH_CATCH ||
            block -> Tag() == AstBlock::TRY_CLAUSE_WITH_FINALLY)
        {
            method_stack -> HandlerRangeStart(enclosing_level).
                Push(code_attribute -> CodeLength() + width);
        }
        else if (block -> Tag() == AstBlock::ABRUPT_TRY_FINALLY)
        {
            method_stack -> HandlerRangeStart(enclosing_level).
                Push(code_attribute -> CodeLength());
            return false;
        }
    }
    return true;
}

void ByteCode::EmitBranch(Opcode opc, Label& lab, AstStatement* over)
{
    // Use the number of tokens as a heuristic for the size of the statement
    // we're jumping over. If the statement is large enough, either change
    // to the 4-byte branch opcode or write out a branch around a goto_w for
    // branch opcodes that don't have a long form.
    //
    // NOTE: We do NOT pop types from StackMapGenerator for branch operands here
    // because the operands (e.g., comparison values from EmitExpression) are not
    // tracked in the StackMapGenerator. Only method call arguments are tracked,
    // and popping here would incorrectly remove those tracked types.
    int sizeHeuristic = over ? over -> RightToken() - over -> LeftToken() : 0;
    if (sizeHeuristic < TOKEN_WIDTH_REQUIRING_GOTOW) {
        PutOp(opc);
        UseLabel(lab, 2, 1);
        return;
    }
    if (opc == OP_GOTO) {
        PutOp(OP_GOTO_W);
        UseLabel(lab, 4, 1);
        return;
    }
    if (opc == OP_JSR) {
        PutOp(OP_JSR_W);
        UseLabel(lab, 4, 1);
        return;
    }
    // if op lab
    //  =>
    // if !op label2
    // goto_w lab
    // label2:
    PutOp(InvertIfOpCode(opc));
    Label label2;
    UseLabel(label2, 2, 1);
    PutOp(OP_GOTO_W);
    UseLabel(lab, 4, 1);
    DefineLabel(label2);
    CompleteLabel(label2);
}

//
// java provides a variety of conditional branch instructions, so
// that a number of operators merit special handling:
//      constant operand
//      negation (we eliminate it)
//      equality
//      ?: && and || (partial evaluation)
//      comparisons
// Other expressions are just evaluated and the appropriate
// branch emitted.
//
// TODO: return a bool that is true if the statement being branched over is
// even needed (if statements and other places might have a constant false
// expression, allowing the next block of code to be skipped entirely).
//
void ByteCode::EmitBranchIfExpression(AstExpression* p, bool cond, Label& lab,
                                      AstStatement* over)
{
    p = StripNops(p);
    assert(p -> Type() == control.boolean_type ||
           p -> Type() == control.Boolean());  // Allow Boolean with auto-unboxing

    // Handle Boolean unboxing: emit call to booleanValue()
    if (p -> Type() == control.Boolean())
    {
        EmitExpression(p);
        PutOp(OP_INVOKEVIRTUAL);
        PutU2(RegisterLibraryMethodref(control.Boolean_booleanValueMethod()));
        if (cond)
            EmitBranch(OP_IFNE, lab, over);  // branch if true
        else
            EmitBranch(OP_IFEQ, lab, over);  // branch if false
        return;
    }

    if (p -> IsConstant())
    {
        if (IsZero(p) != cond)
            EmitBranch(OP_GOTO, lab, over);
        return;
    }

    AstPreUnaryExpression* pre = p -> PreUnaryExpressionCast();
    if (pre) // must be !
    {
        //
        // branch_if(!e,c,l) => branch_if(e,!c,l)
        //
        assert(pre -> Tag() == AstPreUnaryExpression::NOT);
        EmitBranchIfExpression(pre -> expression, ! cond, lab, over);
        return;
    }

    AstConditionalExpression* conditional = p -> ConditionalExpressionCast();
    if (conditional)
    {
        if (conditional -> test_expression -> IsConstant())
        {
            //
            // branch_if(true?a:b, cond, lab) => branch_if(a, cond, lab);
            // branch_if(false?a:b, cond, lab) => branch_if(b, cond, lab);
            //
            EmitBranchIfExpression((IsZero(conditional -> test_expression)
                                    ? conditional -> false_expression
                                    : conditional -> true_expression),
                                   cond, lab, over);
        }
        else if (IsOne(conditional -> true_expression))
        {
            //
            // branch_if(expr?true:true, c, l) => expr, branch if c
            // branch_if(expr?true:false, c, l) => branch_if(expr, c, l);
            // branch_if(expr?true:b, c, l) => branch_if(expr || b, c, l);
            //
            if (IsOne(conditional -> false_expression))
            {
                EmitExpression(conditional -> test_expression, false);
                if (cond)
                    EmitBranch(OP_GOTO, lab, over);
            }
            else if (IsZero(conditional -> false_expression))
            {
                EmitBranchIfExpression(conditional -> test_expression,
                                       cond, lab, over);
            }
            else if (cond)
            {
                EmitBranchIfExpression(conditional -> test_expression, true,
                                       lab, over);
                EmitBranchIfExpression(conditional -> false_expression, true,
                                       lab, over);
            }
            else
            {
                Label skip;
                EmitBranchIfExpression(conditional -> test_expression, true,
                                       skip, over);
                EmitBranchIfExpression(conditional -> false_expression, false,
                                       lab, over);
                DefineLabel(skip);
                CompleteLabel(skip);
            }
        }
        else if (IsZero(conditional -> true_expression))
        {
            //
            // branch_if(expr?false:true, c, l) => branch_if(expr, ! c, l);
            // branch_if(expr?false:false, c, l) => expr, branch if ! c
            // branch_if(expr?false:b, c, l) => branch_if(!expr && b, c, l);
            //
            if (IsOne(conditional -> false_expression))
            {
                EmitBranchIfExpression(conditional -> test_expression,
                                       ! cond, lab, over);
            }
            else if (IsZero(conditional -> false_expression))
            {
                EmitExpression(conditional -> test_expression, false);
                if (! cond)
                    EmitBranch(OP_GOTO, lab, over);
            }
            else if (! cond)
            {
                EmitBranchIfExpression(conditional -> test_expression, true,
                                       lab, over);
                EmitBranchIfExpression(conditional -> false_expression, false,
                                       lab, over);
            }
            else
            {
                Label skip;
                EmitBranchIfExpression(conditional -> test_expression, true,
                                       skip, over);
                EmitBranchIfExpression(conditional -> false_expression, true,
                                       lab, over);
                DefineLabel(skip);
                CompleteLabel(skip);
            }
        }
        else if (IsOne(conditional -> false_expression))
        {
            //
            // branch_if(expr?a:true, c, l) => branch_if(!expr || a, c, l);
            //
            if (cond)
            {
                EmitBranchIfExpression(conditional -> test_expression, false,
                                       lab, over);
                EmitBranchIfExpression(conditional -> true_expression, true,
                                       lab, over);
            }
            else
            {
                Label skip;
                EmitBranchIfExpression(conditional -> test_expression, false,
                                       skip, over);
                EmitBranchIfExpression(conditional -> true_expression, false,
                                       lab, over);
                DefineLabel(skip);
                CompleteLabel(skip);
            }
        }
        else if (IsZero(conditional -> false_expression))
        {
            //
            // branch_if(expr?a:false, c, l) => branch_if(expr && a, c, l);
            //
            if (! cond)
            {
                EmitBranchIfExpression(conditional -> test_expression, false,
                                       lab, over);
                EmitBranchIfExpression(conditional -> true_expression, false,
                                       lab, over);
            }
            else
            {
                Label skip;
                EmitBranchIfExpression(conditional -> test_expression, false,
                                       skip, over);
                EmitBranchIfExpression(conditional -> true_expression, true,
                                       lab, over);
                DefineLabel(skip);
                CompleteLabel(skip);
            }
        }
        else
        {
            //
            // branch_if(expr?a:b, c, l) =>
            //   branch_if(expr, false, lab1)
            //   branch_if(a, c, l)
            //   goto lab2
            //   lab1: branch_if(b, c, l)
            //   lab2:
            //
            Label lab1, lab2;
            EmitBranchIfExpression(conditional -> test_expression, false, lab1,
                                   over);
            EmitBranchIfExpression(conditional -> true_expression, cond, lab,
                                   over);
            EmitBranch(OP_GOTO, lab2, over);
            DefineLabel(lab1);
            CompleteLabel(lab1);
            EmitBranchIfExpression(conditional -> false_expression, cond, lab,
                                   over);
            DefineLabel(lab2);
            CompleteLabel(lab2);
        }
        return;
    }

    AstInstanceofExpression* instanceof = p -> InstanceofExpressionCast();
    if (instanceof)
    {
        AstExpression* expr = StripNops(instanceof -> expression);
        TypeSymbol* left_type = expr -> Type();
        TypeSymbol* right_type = instanceof -> type -> symbol;
        if (right_type -> num_dimensions > 255)
        {
            semantic.ReportSemError(SemanticError::ARRAY_OVERFLOW,
                                    instanceof -> type);
        }
        if (left_type == control.null_type)
        {
            //
            // We know the result: false. But emit the left expression,
            // in case of side effects in (expr ? null : null).
            //
            EmitExpression(expr, false);
            if (! cond)
                EmitBranch(OP_GOTO, lab, over);
        }
        else if (expr -> IsConstant() || // a String constant
                 expr -> BinaryExpressionCast()) // a String concat
        {
            //
            // We know the result: true, since the expression is non-null
            // and String is a final class.
            //
            assert(left_type == control.String());
            EmitExpression(expr, false);
            if (cond)
                EmitBranch(OP_GOTO, lab, over);
        }
        else if ((expr -> ThisExpressionCast() ||
                  expr -> SuperExpressionCast() ||
                  expr -> ClassLiteralCast() ||
                  expr -> ClassCreationExpressionCast() ||
                  expr -> ArrayCreationExpressionCast()) &&
                 left_type -> IsSubtype(right_type))
        {
            //
            // We know the result: true, since the expression is non-null.
            //
            EmitExpression(expr, false);
            if (cond)
                EmitBranch(OP_GOTO, lab, over);
        }
        else
        {
            EmitExpression(expr);
            PutOp(OP_INSTANCEOF);
            PutU2(RegisterClass(right_type));
            EmitBranch((cond ? OP_IFNE : OP_IFEQ), lab, over);
        }
        return;
    }

    //
    // dispose of non-binary expression case by just evaluating
    // operand and emitting appropiate test.
    //
    AstBinaryExpression* bp = p -> BinaryExpressionCast();
    if (! bp)
    {
        EmitExpression(p);
        EmitBranch((cond ? OP_IFNE : OP_IFEQ), lab, over);
        return;
    }

    //
    // Here if binary expression, so extract operands
    //
    AstExpression* left = StripNops(bp -> left_expression);
    AstExpression* right = StripNops(bp -> right_expression);

    TypeSymbol* left_type = left -> Type();
    TypeSymbol* right_type = right -> Type();
    switch (bp -> Tag())
    {
    case AstBinaryExpression::AND_AND:
        //
        // branch_if(true&&b, cond, lab) => branch_if(b, cond, lab);
        // branch_if(false&&b, cond, lab) => branch_if(false, cond, lab);
        //
        if (left -> IsConstant())
        {
            if (IsOne(left))
                EmitBranchIfExpression(right, cond, lab, over);
            else if (! cond)
                EmitBranch(OP_GOTO, lab, over);
        }
        //
        // branch_if(a&&true, cond, lab) => branch_if(a, cond, lab);
        // branch_if(a&&false, cond, lab) => emit(a), pop; for side effects
        //
        else if (right -> IsConstant())
        {
            if (IsOne(right))
                EmitBranchIfExpression(left, cond, lab, over);
            else
            {
                EmitExpression(left, false);
                if (! cond)
                    EmitBranch(OP_GOTO, lab, over);
            }
        }
        //
        // branch_if(a&&b, true, lab) =>
        //   branch_if(a,false,skip);
        //   branch_if(b,true,lab);
        //   skip:
        // branch_if(a&&b, false, lab) =>
        //   branch_if(a,false,lab);
        //   branch_if(b,false,lab);
        //
        else if (cond)
        {
            Label skip;
            EmitBranchIfExpression(left, false, skip, over);
            EmitBranchIfExpression(right, true, lab, over);
            DefineLabel(skip);
            CompleteLabel(skip);
        }
        else
        {
            EmitBranchIfExpression(left, false, lab, over);
            EmitBranchIfExpression(right, false, lab, over);
        }
        return;
    case AstBinaryExpression::OR_OR:
        //
        // branch_if(false||b, cond, lab) => branch_if(b, cond, lab);
        // branch_if(true||b, cond, lab) => branch_if(true, cond, lab);
        //
        if (left -> IsConstant())
        {
            if (IsZero(left))
                EmitBranchIfExpression(right, cond, lab, over);
            else if (cond)
                EmitBranch(OP_GOTO, lab, over);
        }
        //
        // branch_if(a||false, cond, lab) => branch_if(a, cond, lab);
        // branch_if(a||true, cond, lab) => emit(a), pop; for side effects
        //
        else if (right -> IsConstant())
        {
            if (IsZero(right))
                EmitBranchIfExpression(left, cond, lab, over);
            else
            {
                EmitExpression(left, false);
                if (cond)
                    EmitBranch(OP_GOTO, lab, over);
            }
        }
        //
        // branch_if(a||b,true,lab) =>
        //   branch_if(a,true,lab);
        //   branch_if(b,true,lab);
        // branch_if(a||b,false,lab) =>
        //   branch_if(a,true,skip);
        //   branch_if(b,false,lab);
        //   skip:
        //
        else if (cond)
        {
            EmitBranchIfExpression(left, true, lab, over);
            EmitBranchIfExpression(right, true, lab, over);
        }
        else
        {
            Label skip;
            EmitBranchIfExpression(left, true, skip, over);
            EmitBranchIfExpression(right, false, lab, over);
            DefineLabel(skip);
            CompleteLabel(skip);
        }
        return;
    case AstBinaryExpression::XOR: // ^ on booleans is equavalent to !=
        assert(left_type == control.boolean_type);
        // Fallthrough!
    case AstBinaryExpression::EQUAL_EQUAL:
    case AstBinaryExpression::NOT_EQUAL:
        //
        // One of the operands is null. We must evaluate both operands, to get
        // any side effects in (expr ? null : null).
        //
        if (left_type == control.null_type || right_type == control.null_type)
        {
            EmitExpression(left, left_type != control.null_type);
            EmitExpression(right, right_type != control.null_type);
            if (left_type == right_type)
            {
                if (cond == (bp -> Tag() == AstBinaryExpression::EQUAL_EQUAL))
                {
                    EmitBranch(OP_GOTO, lab, over);
                }
            }
            else
            {
                if (bp -> Tag() == AstBinaryExpression::EQUAL_EQUAL)
                    EmitBranch(cond ? OP_IFNULL : OP_IFNONNULL, lab, over);
                else EmitBranch(cond ? OP_IFNONNULL : OP_IFNULL, lab, over);
            }
            return;
        }

        //
        // One of the operands is true. Branch on the other.
        //
        if (left_type == control.boolean_type &&
            (IsOne(left) || IsOne(right)))
        {
            EmitBranchIfExpression(IsOne(left) ? right : left,
                                   cond == (bp -> Tag() == AstBinaryExpression::EQUAL_EQUAL),
                                   lab, over);
            return;
        }

        //
        // Both operands are integer.
        //
        if (control.IsSimpleIntegerValueType(left_type) ||
             left_type == control.boolean_type)
        {
            assert(control.IsSimpleIntegerValueType(right_type) ||
                   right_type == control.boolean_type);

            if (IsZero(left) || IsZero(right))
            {
                if (left_type == control.boolean_type)
                {
                    //
                    // One of the operands is false. Branch on the other.
                    //
                    EmitBranchIfExpression(IsZero(left) ? right : left,
                                           cond == (bp -> Tag() != AstBinaryExpression::EQUAL_EQUAL),
                                           lab, over);
                }
                else
                {
                    //
                    // One of the operands is zero. Only emit the other.
                    //
                    EmitExpression(IsZero(left) ? right : left);

                    if (bp -> Tag() == AstBinaryExpression::EQUAL_EQUAL)
                        EmitBranch((cond ? OP_IFEQ : OP_IFNE), lab, over);
                    else EmitBranch((cond ? OP_IFNE : OP_IFEQ), lab, over);
                }
            }
            else
            {
                EmitExpression(left);
                EmitExpression(right);

                if (bp -> Tag() == AstBinaryExpression::EQUAL_EQUAL)
                    EmitBranch((cond ? OP_IF_ICMPEQ : OP_IF_ICMPNE), lab, over);
                else
                    EmitBranch((cond ? OP_IF_ICMPNE : OP_IF_ICMPEQ), lab, over);
            }

            return;
        }

        //
        // Both operands are reference types: just do the comparison.
        //
        if (IsReferenceType(left_type))
        {
            assert(IsReferenceType(right_type));
            EmitExpression(left);
            EmitExpression(right);

            if (bp -> Tag() == AstBinaryExpression::EQUAL_EQUAL)
                EmitBranch((cond ? OP_IF_ACMPEQ : OP_IF_ACMPNE), lab, over);
            else EmitBranch((cond ? OP_IF_ACMPNE : OP_IF_ACMPEQ), lab, over);

            return;
        }

        break;
    case AstBinaryExpression::IOR:
        //
        // One argument is false. Branch on other.
        //
        if (IsZero(left) || IsZero(right))
        {
            EmitBranchIfExpression(IsZero(left) ? right : left,
                                   cond, lab, over);
            return;
        }

        //
        // One argument is true. Emit the other, and result is true.
        //
        if (IsOne(left) || IsOne(right))
        {
            EmitExpression(IsOne(left) ? right : left, false);
            if (cond)
                EmitBranch(OP_GOTO, lab, over);
            return;
        }
        break;
    case AstBinaryExpression::AND:
        //
        // One argument is true. Branch on other.
        //
        if (IsOne(left) || IsOne(right))
        {
            EmitBranchIfExpression(IsOne(left) ? right : left,
                                   cond, lab, over);
            return;
        }

        //
        // One argument is false. Emit the other, and result is false.
        //
        if (IsZero(left) || IsZero(right))
        {
            EmitExpression(IsZero(left) ? right : left, false);
            if (! cond)
                EmitBranch(OP_GOTO, lab, over);
            return;
        }
        break;
    default:
        break;
    }

    //
    // here if not comparison, comparison for non-integral numeric types, or
    // integral comparison for which no special casing needed.
    // Begin by dealing with non-comparisons
    //
    switch (bp -> Tag())
    {
    case AstBinaryExpression::LESS:
    case AstBinaryExpression::LESS_EQUAL:
    case AstBinaryExpression::GREATER:
    case AstBinaryExpression::GREATER_EQUAL:
    case AstBinaryExpression::EQUAL_EQUAL:
    case AstBinaryExpression::NOT_EQUAL:
        break; // break to continue comparison processing
    default:
        //
        // not a comparison, get the (necessarily boolean) value
        // of the expression and branch on the result
        //
        EmitExpression(p);
        EmitBranch(cond ? OP_IFNE : OP_IFEQ, lab, over);
        return;
    }

    //
    //
    //
    Opcode opcode = OP_NOP,
           op_true,
           op_false;
    assert(left_type != control.boolean_type);
    if (control.IsSimpleIntegerValueType(left_type))
    {
        //
        // we have already dealt with EQUAL_EQUAL and NOT_EQUAL for the case
        // of two integers, but still need to look for comparisons for which
        // one operand may be zero.
        //
        if (IsZero(left))
        {
            EmitExpression(right);
            switch (bp -> Tag())
            {
            case AstBinaryExpression::LESS:
                // if (0 < x) same as  if (x > 0)
                op_true = OP_IFGT;
                op_false = OP_IFLE;
                break;
            case AstBinaryExpression::LESS_EQUAL:
                // if (0 <= x) same as if (x >= 0)
                op_true = OP_IFGE;
                op_false = OP_IFLT;
                break;
            case AstBinaryExpression::GREATER:
                // if (0 > x) same as if (x < 0)
                op_true = OP_IFLT;
                op_false = OP_IFGE;
                break;
            case AstBinaryExpression::GREATER_EQUAL:
                // if (0 >= x) same as if (x <= 0)
                op_true = OP_IFLE;
                op_false = OP_IFGT;
                break;
            default:
                assert(false);
                break;
            }
        }
        else if (IsZero(right))
        {
            EmitExpression(left);

            switch (bp -> Tag())
            {
            case AstBinaryExpression::LESS:
                op_true = OP_IFLT;
                op_false = OP_IFGE;
                break;
            case AstBinaryExpression::LESS_EQUAL:
                op_true = OP_IFLE;
                op_false = OP_IFGT;
                break;
            case AstBinaryExpression::GREATER:
                op_true = OP_IFGT;
                op_false = OP_IFLE;
                break;
            case AstBinaryExpression::GREATER_EQUAL:
                op_true = OP_IFGE;
                op_false = OP_IFLT;
                break;
            default:
                assert(false);
                break;
            }
        }
        else
        {
            EmitExpression(left);
            EmitExpression(right);

            switch (bp -> Tag())
            {
            case AstBinaryExpression::LESS:
                op_true = OP_IF_ICMPLT;
                op_false = OP_IF_ICMPGE;
                break;
            case AstBinaryExpression::LESS_EQUAL:
                op_true = OP_IF_ICMPLE;
                op_false = OP_IF_ICMPGT;
                break;
            case AstBinaryExpression::GREATER:
                op_true = OP_IF_ICMPGT;
                op_false = OP_IF_ICMPLE;
                break;
            case AstBinaryExpression::GREATER_EQUAL:
                op_true = OP_IF_ICMPGE;
                op_false = OP_IF_ICMPLT;
                break;
            default:
                assert(false);
                break;
            }
        }
    }
    else if (left_type == control.long_type)
    {
        EmitExpression(left);
        EmitExpression(right);

        opcode = OP_LCMP;

        //
        // branch according to result value on stack
        //
        switch (bp -> Tag())
        {
        case AstBinaryExpression::EQUAL_EQUAL:
            op_true = OP_IFEQ;
            op_false = OP_IFNE;
            break;
        case AstBinaryExpression::NOT_EQUAL:
            op_true = OP_IFNE;
            op_false = OP_IFEQ;
            break;
        case AstBinaryExpression::LESS:
            op_true = OP_IFLT;
            op_false = OP_IFGE;
            break;
        case AstBinaryExpression::LESS_EQUAL:
            op_true = OP_IFLE;
            op_false = OP_IFGT;
            break;
        case AstBinaryExpression::GREATER:
            op_true = OP_IFGT;
            op_false = OP_IFLE;
            break;
        case AstBinaryExpression::GREATER_EQUAL:
            op_true = OP_IFGE;
            op_false = OP_IFLT;
            break;
        default:
            assert(false);
            break;
        }
    }
    else if (left_type == control.float_type)
    {
        EmitExpression(left);
        EmitExpression(right);

        switch (bp -> Tag())
        {
        case AstBinaryExpression::EQUAL_EQUAL:
            opcode = OP_FCMPL;
            op_true = OP_IFEQ;
            op_false = OP_IFNE;
            break;
        case AstBinaryExpression::NOT_EQUAL:
            opcode = OP_FCMPL;
            op_true = OP_IFNE;
            op_false = OP_IFEQ;
            break;
        case AstBinaryExpression::LESS:
            opcode = OP_FCMPG;
            op_true = OP_IFLT;
            op_false = OP_IFGE;
            break;
        case AstBinaryExpression::LESS_EQUAL:
            opcode = OP_FCMPG;
            op_true = OP_IFLE;
            op_false = OP_IFGT;
            break;
        case AstBinaryExpression::GREATER:
            opcode = OP_FCMPL;
            op_true = OP_IFGT;
            op_false = OP_IFLE;
            break;
        case AstBinaryExpression::GREATER_EQUAL:
            opcode = OP_FCMPL;
            op_true = OP_IFGE;
            op_false = OP_IFLT;
            break;
        default:
            assert(false);
            break;
        }
    }
    else if (left_type == control.double_type)
    {
        EmitExpression(left);
        EmitExpression(right);
        switch (bp -> Tag())
        {
        case AstBinaryExpression::EQUAL_EQUAL:
            opcode = OP_DCMPL;
            op_true = OP_IFEQ;
            op_false = OP_IFNE;
            break;
        case AstBinaryExpression::NOT_EQUAL:
            opcode = OP_DCMPL;
            op_true = OP_IFNE;
            op_false = OP_IFEQ;
            break;
        case AstBinaryExpression::LESS:
            opcode = OP_DCMPG;
            op_true = OP_IFLT;
            op_false = OP_IFGE;
            break;
        case AstBinaryExpression::LESS_EQUAL:
            opcode = OP_DCMPG;
            op_true = OP_IFLE;
            op_false = OP_IFGT;
            break;
        case AstBinaryExpression::GREATER:
            opcode = OP_DCMPL;
            op_true = OP_IFGT;
            op_false = OP_IFLE;
            break;
        case AstBinaryExpression::GREATER_EQUAL:
            opcode = OP_DCMPL;
            op_true = OP_IFGE;
            op_false = OP_IFLT;
            break;
        default:
            assert(false);
            break;
        }
    }
    else assert(false && "comparison of unsupported type");

    if (opcode != OP_NOP)
        PutOp(opcode); // if need to emit comparison before branch

    EmitBranch (cond ? op_true : op_false, lab, over);
}


//
// Emits a synchronized statement, including monitor cleanup. The return
// value is true if the contained statement is abrupt.
//
bool ByteCode::EmitSynchronizedStatement(AstSynchronizedStatement* statement)
{
    int variable_index =
        method_stack -> TopBlock() -> block_symbol -> helper_variable_index;

    Label start_label;
    //
    // This code must be careful of asynchronous exceptions. Even if the
    // synchronized block is empty, user code can use Thread.stop(Throwable),
    // so we must ensure the monitor exits. We make sure that all instructions
    // after the monitorenter are covered.  By sticking the catchall code
    // before the synchronized block, we can even make abrupt exits inside the
    // statement be asynch-exception safe.  Note that the user can cause
    // deadlock (ie. an infinite loop), by releasing the monitor (via JNI or
    // some other means) in the block statement, so that the monitorexit fails
    // synchronously with an IllegalMonitorStateException and tries again; but
    // JLS 17.13 states that the compiler need not worry about such user
    // stupidity.
    //
    EmitBranch(OP_GOTO, start_label, NULL);
    u2 handler_pc = code_attribute -> CodeLength();

    // Record StackMapTable frame for synchronized exception handler
    if (stack_map_generator)
    {
        stack_map_generator->ClearStack();
        stack_map_generator->PushType(control.Throwable());
        stack_map_generator->RecordFrame(handler_pc);
        stack_map_generator->ClearStack(); // Reset for subsequent non-handler code
    }

    assert(stack_depth == 0);
    stack_depth = 1; // account for the exception already on the stack
    LoadLocal(variable_index, control.Object()); // reload monitor
    PutOp(OP_MONITOREXIT);
    u2 throw_pc = code_attribute -> CodeLength();
    PutOp(OP_ATHROW);
    code_attribute -> AddException(handler_pc, throw_pc, handler_pc, 0);

    //
    // Even if enclosed statement is a nop, we must enter the monitor, because
    // of memory flushing side effects of synchronization.
    //
    DefineLabel(start_label);
    CompleteLabel(start_label);
    EmitExpression(statement -> expression);
    PutOp(OP_DUP); // duplicate for saving, entering monitor
    StoreLocal(variable_index, control.Object()); // save address of object
    PutOp(OP_MONITORENTER); // enter monitor associated with object

    assert(method_stack -> TopHandlerRangeStart().Length() == 0 &&
           method_stack -> TopHandlerRangeEnd().Length() == 0);
    method_stack -> TopHandlerRangeStart().Push(code_attribute -> CodeLength());
    bool abrupt = EmitBlockStatement(statement -> block);

    if (! abrupt)
    {
        LoadLocal(variable_index, control.Object()); // reload monitor
        PutOp(OP_MONITOREXIT);
    }
    u2 end_pc = code_attribute -> CodeLength();
    method_stack -> TopHandlerRangeEnd().Push(end_pc);
    unsigned count = method_stack -> TopHandlerRangeStart().Length();
    assert(count == method_stack -> TopHandlerRangeEnd().Length());
    while (count--)
    {
        code_attribute ->
            AddException(method_stack -> TopHandlerRangeStart().Pop(),
                         method_stack -> TopHandlerRangeEnd().Pop(),
                         handler_pc, 0);
    }
    return abrupt;
}


void ByteCode::EmitAssertStatement(AstAssertStatement* assertion)
{
    //
    // When constant true, the assert statement is a no-op.
    // Otherwise, assert a : b; is syntactic sugar for:
    //
    // while (! ($noassert && (a)))
    //     throw new java.lang.AssertionError(b);
    //
    if (semantic.IsConstantTrue(assertion -> condition) ||
        control.option.noassert ||
        control.option.target < JopaOption::SDK1_4)
    {
        return;
    }
    PutOp(OP_GETSTATIC);
    PutU2(RegisterFieldref(assertion -> assert_variable));
    Label label;
    EmitBranch(OP_IFNE, label);
    EmitBranchIfExpression(assertion -> condition, true, label);
    PutOp(OP_NEW);
    PutU2(RegisterClass(control.AssertionError()));
    PutOp(OP_DUP);

    MethodSymbol* constructor = NULL;
    if (assertion -> message_opt)
    {
        EmitExpression(assertion -> message_opt);
        TypeSymbol* type = assertion -> message_opt -> Type();
        if (! control.AssertionError() -> Bad())
        {
            // We found the class, now can we find the method?
            if (type == control.char_type)
                constructor = control.AssertionError_InitWithCharMethod();
            else if (type == control.boolean_type)
                constructor = control.AssertionError_InitWithBooleanMethod();
            else if (type == control.int_type || type == control.short_type ||
                     type == control.byte_type)
            {
                constructor = control.AssertionError_InitWithIntMethod();
            }
            else if (type == control.long_type)
                constructor = control.AssertionError_InitWithLongMethod();
            else if (type == control.float_type)
                constructor = control.AssertionError_InitWithFloatMethod();
            else if (type == control.double_type)
                constructor = control.AssertionError_InitWithDoubleMethod();
            else if (type == control.null_type || IsReferenceType(type))
                constructor = control.AssertionError_InitWithObjectMethod();
            else assert (false && "Missing AssertionError constructor!");
            if (! constructor) // We didn't find it; suckage....
                // TODO: error ought to include what we were looking for
                semantic.ReportSemError(SemanticError::LIBRARY_METHOD_NOT_FOUND,
                                        assertion,
                                        unit_type -> ContainingPackageName(),
                                        unit_type -> ExternalName());
        }
        else
        {
            // The type for AssertionError is BAD, that means it wasn't
            // found! but the calls to control.AssertionError() above will
            // file a semantic error for us, no need to here.
        }
        ChangeStack(- GetTypeWords(type));
    }
    else constructor = control.AssertionError_InitMethod();

    PutOp(OP_INVOKESPECIAL);
    PutU2(RegisterLibraryMethodref(constructor));
    PutOp(OP_ATHROW);
    DefineLabel(label);
    CompleteLabel(label);
}


void ByteCode::EmitForeachStatement(AstForeachStatement* foreach)
{
    int helper_index =
        method_stack -> TopBlock() -> block_symbol -> helper_variable_index;
    bool abrupt;
    EmitExpression(foreach -> expression);
    Label loop;
    Label& comp = method_stack -> TopContinueLabel();
    Label end;
    TypeSymbol* expr_type = foreach -> expression -> Type();
    VariableSymbol* var =
        foreach -> formal_parameter -> formal_declarator -> symbol;
    TypeSymbol* component_type = var -> Type();
    // Saved locals state for the end label frame (captured before loop body)
    Tuple<StackMapGenerator::VerificationType>* end_label_locals = NULL;
    if (expr_type -> IsArray())
    {
        //
        // Turn 'l: for(a b : c) d' into
        // { expr_type #0 = c;
        //   int #1 = #0.length;
        //   l: for(int #2 = 0; #2 < #1; #2++) {
        //     a b = #0[#2];
        //     d; }}
        // Or in bytecode:
        // eval c onto stack
        // dup
        // astore helper_index
        // arraylength
        // dup
        // istore helper_index+1
        // ifeq end
        // iconst_0
        // istore helper_index+2
        // iconst_0
        // loop:
        // aload helper_index
        // swap
        // xaload (for x = b, s, i, l, c, f, d, a)
        // assignment-conversion (if necessary)
        // xstore b (for x = i, l, f, d, a)
        // eval d (continue to comp, break to end)
        // comp:
        // iinc helper_index+2, 1
        // iload helper_index+1
        // iload helper_index+2
        // dup_x1
        // if_icmpgt loop
        // pop
        // end:
        //
        TypeSymbol* expr_subtype = expr_type -> ArraySubtype();
        if (IsNop(foreach -> statement) &&
            (! component_type -> Primitive() || expr_subtype -> Primitive()))
        {
            //
            // Optimization (arrays only): no need to increment loop counter
            // if nothing is done in the loop; and we simply check that the
            // array is non-null from arraylength. But beware of autounboxing,
            // which can cause NullPointerException.
            //
            PutOp(OP_ARRAYLENGTH);
            PutOp(OP_POP);
            return;
        }
        PutOp(OP_DUP);
        StoreLocal(helper_index, expr_type);
        PutOp(OP_ARRAYLENGTH);
        PutOp(OP_DUP);
        StoreLocal(helper_index + 1, control.int_type);
        // Initialize loop counter before the branch so all paths have consistent locals
        PutOp(OP_ICONST_0);
        StoreLocal(helper_index + 2, control.int_type);
        // Save state for the end label frame (before loop body modifies anything)
        if (stack_map_generator)
        {
            end_label_locals = stack_map_generator->SaveLocals();
        }
        EmitBranch(OP_IFEQ, end);
        PutOp(OP_ICONST_0);
        // Synchronize stack_map_generator for loop label - stack has [current_index]
        // Explicitly record frame for loop header (backward branch target)
        if (stack_map_generator)
        {
            stack_map_generator->ClearStack();
            stack_map_generator->PushType(control.int_type);
            // Set loop variable to TOP since it's not yet assigned at loop entry
            stack_map_generator->SetLocal(var -> LocalVariableIndex(),
                StackMapGenerator::VerificationType(
                    StackMapGenerator::VerificationType::TYPE_Top));
            // Record frame at loop header for backward branch verification
            stack_map_generator->RecordFrame(code_attribute -> CodeLength());
        }
        DefineLabel(loop);
        LoadLocal(helper_index, expr_type);
        PutOp(OP_SWAP);
        LoadArrayElement(expr_type -> ArraySubtype());
        EmitCast(component_type, expr_type -> ArraySubtype());
        u2 var_pc = code_attribute -> CodeLength();
        StoreLocal(var -> LocalVariableIndex(), component_type);
        // Clear stack state before loop body - it should be empty at this point
        if (stack_map_generator)
            stack_map_generator->ClearStack();
        abrupt = EmitStatement(foreach -> statement);
        if (control.option.g & JopaOption::VARS)
        {
            local_variable_table_attribute ->
                AddLocalVariable(var_pc, code_attribute -> CodeLength(),
                                 RegisterName(var -> ExternalIdentity()),
                                 RegisterUtf8(component_type -> signature),
                                 var -> LocalVariableIndex());
        }
        if (! abrupt || foreach -> statement -> can_complete_normally)
        {
            DefineLabel(comp);
            PutOpIINC(helper_index + 2, 1);
            LoadLocal(helper_index + 1, control.int_type);
            LoadLocal(helper_index + 2, control.int_type);
            PutOp(OP_DUP_X1);
            EmitBranch(OP_IF_ICMPGT, loop);
            PutOp(OP_POP);
        }
    }
    else
    {
        assert(foreach -> expression -> Type() ->
               IsSubtype(control.Iterable()));
        //
        // Turn 'l: for(a b : c) d' into
        // for(java.util.Iterator #0 = c.iterator(); #0.hasNext();) {
        //   a b = (a) c.next();
        //   d; }
        // Or in bytecode:
        // eval c onto stack
        // invokeinterface java.lang.Iterable.iterator()Ljava/util/Iterator;
        // dup
        // invokeinterface java.util.Iterator.hasNext()Z
        // ifeq cleanup
        // dup
        // astore helper_index
        // loop:
        // invokeinterface java.util.Iterator.next()Ljava/lang/Object;
        // checkcast a
        // astore b
        // eval d (continue to comp, break to end)
        // comp:
        // aload helper_index
        // dup
        // invokeinterface java.util.Iterator.hasNext()Z
        // ifne loop
        // cleanup:
        // pop
        // end:
        //
        Label cleanup;
        PutOp(OP_INVOKEINTERFACE);
        PutU2(RegisterLibraryMethodref(control.Iterable_iteratorMethod()));
        PutU1(1);
        PutU1(0);
        ChangeStack(1);
        PutOp(OP_DUP);
        PutOp(OP_INVOKEINTERFACE);
        u2 hasNext_index =
            RegisterLibraryMethodref(control.Iterator_hasNextMethod());
        PutU2(hasNext_index);
        PutU1(1);
        PutU1(0);
        ChangeStack(1);
        // Save locals state for the end label frame (before iterator and loop variable are assigned)
        // At this point stack = [Iterator, boolean], IFEQ will pop the boolean
        if (stack_map_generator && !end_label_locals)
        {
            end_label_locals = stack_map_generator->SaveLocals();
        }
        EmitBranch(OP_IFEQ, cleanup);
        // After IFEQ (not taken): Stack = [Iterator]
        PutOp(OP_DUP);
        StoreLocal(helper_index, control.Iterator());
        // After StoreLocal: Stack = [Iterator]
        // Record frame at loop header for backward branch verification
        // Stack has [Iterator], iterator local is set
        if (stack_map_generator)
        {
            // Explicitly set the correct stack state for the loop header frame
            stack_map_generator->ClearStack();
            stack_map_generator->PushType(control.Iterator());
            // Set loop variable to TOP since it's not yet assigned at loop entry
            stack_map_generator->SetLocal(var -> LocalVariableIndex(),
                StackMapGenerator::VerificationType(
                    StackMapGenerator::VerificationType::TYPE_Top));
            stack_map_generator->RecordFrame(code_attribute -> CodeLength());
        }
        DefineLabel(loop);
        PutOp(OP_INVOKEINTERFACE);
        PutU2(RegisterLibraryMethodref(control.Iterator_nextMethod()));
        PutU1(1);
        PutU1(0);
        ChangeStack(1);
        if (component_type != control.Object())
        {
            PutOp(OP_CHECKCAST);
            PutU2(RegisterClass(component_type));
        }
        u2 var_pc = code_attribute -> CodeLength();
        StoreLocal(var -> LocalVariableIndex(), component_type);
        // Clear stack state before loop body - it should be empty at this point
        if (stack_map_generator)
            stack_map_generator->ClearStack();
        abrupt = EmitStatement(foreach -> statement);
        if (control.option.g & JopaOption::VARS)
        {
            local_variable_table_attribute ->
                AddLocalVariable(var_pc, code_attribute -> CodeLength(),
                                 RegisterName(var -> ExternalIdentity()),
                                 RegisterUtf8(component_type -> signature),
                                 var -> LocalVariableIndex());
        }
        if (! abrupt || foreach -> statement -> can_complete_normally)
        {
            DefineLabel(comp);
            LoadLocal(helper_index, control.Iterator());
            PutOp(OP_DUP);
            PutOp(OP_INVOKEINTERFACE);
            PutU2(hasNext_index);
            PutU1(1);
            PutU1(0);
            ChangeStack(1);
            // Stack: [Iterator, int]; IFNE will pop int, leaving [Iterator]
            // At the backward branch target (loop), we need stack = [Iterator]
            EmitBranch(OP_IFNE, loop);
            // After IFNE (not taken): Stack = [Iterator]
        }
        else ChangeStack(1);
        // Record frame at cleanup label with saved locals (before loop body)
        // This is critical because the early exit branch has only pre-loop-body locals
        // At cleanup, stack has [Iterator] from both paths (early exit and normal exit)
        if (stack_map_generator && end_label_locals)
        {
            // Create a copy of saved locals for the cleanup frame
            Tuple<StackMapGenerator::VerificationType>* cleanup_locals =
                new Tuple<StackMapGenerator::VerificationType>(end_label_locals->Length() + 2);
            for (unsigned i = 0; i < end_label_locals->Length(); i++)
                cleanup_locals->Next() = (*end_label_locals)[i];
            // Stack has [Iterator] at cleanup
            stack_map_generator->RecordFrameWithLocalsAndStack(
                code_attribute->CodeLength(), cleanup_locals, control.Iterator());
            delete cleanup_locals;
        }
        else if (stack_map_generator)
        {
            stack_map_generator->ClearStack();
            stack_map_generator->PushType(control.Iterator());
        }
        DefineLabel(cleanup);
        CompleteLabel(cleanup);
        PutOp(OP_POP);
        // Clear stack after POP for the end label
        if (stack_map_generator)
        {
            stack_map_generator->ClearStack();
        }
    }
    // Record frame at end label with the saved locals (before loop body)
    // This is critical for nested loops - the end label frame should not include
    // variables created by inner loops
    if (stack_map_generator && end_label_locals)
    {
        // Set loop variable to TOP in the saved locals since it's out of scope
        while ((int)end_label_locals->Length() <= var->LocalVariableIndex())
            end_label_locals->Next() = StackMapGenerator::VerificationType(
                StackMapGenerator::VerificationType::TYPE_Top);
        (*end_label_locals)[var->LocalVariableIndex()] =
            StackMapGenerator::VerificationType(
                StackMapGenerator::VerificationType::TYPE_Top);
        // Record the frame with pre-loop-body locals
        stack_map_generator->RecordFrameWithLocals(code_attribute->CodeLength(), end_label_locals);
    }
    // Clean up saved_locals
    if (end_label_locals)
        delete end_label_locals;
    // Sync the StackMapGenerator's state after the foreach ends
    // Set loop variable and helper variables to TOP
    if (stack_map_generator)
    {
        // Set loop variable to TOP
        stack_map_generator->SetLocal(var -> LocalVariableIndex(),
            StackMapGenerator::VerificationType(
                StackMapGenerator::VerificationType::TYPE_Top));
        // Set helper variables to TOP
        stack_map_generator->SetLocal(helper_index,
            StackMapGenerator::VerificationType(
                StackMapGenerator::VerificationType::TYPE_Top));
        if (expr_type -> IsArray())
        {
            stack_map_generator->SetLocal(helper_index + 1,
                StackMapGenerator::VerificationType(
                    StackMapGenerator::VerificationType::TYPE_Top));
            stack_map_generator->SetLocal(helper_index + 2,
                StackMapGenerator::VerificationType(
                    StackMapGenerator::VerificationType::TYPE_Top));
        }
        // Truncate any locals declared inside the loop body (after the loop variable)
        // This ensures variables like "Integer i" are out of scope after the loop
        stack_map_generator->TruncateLocals(var -> LocalVariableIndex() + 1);
        stack_map_generator->ClearStack();
    }
    DefineLabel(end);
    CompleteLabel(loop);
    CompleteLabel(comp);
    CompleteLabel(end);
}

} // Close namespace Jopa block
