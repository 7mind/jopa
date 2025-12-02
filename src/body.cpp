#include "platform.h"
#include "semantic.h"
#include "control.h"
#include "option.h"
#include "stream.h"
#include "paramtype.h"
#include "typeparam.h"


namespace Jopa { // Open namespace Jopa block
void Semantic::ProcessBlockStatements(AstBlock* block_body)
{
    //
    // An empty block that is not a switch block can complete normally
    // iff it is reachable. A nonempty block that is not a switch
    // block can complete normally iff the last statement in it can
    // complete normally.
    //
    if (block_body -> NumStatements() == 0)
        block_body -> can_complete_normally = block_body -> is_reachable;
    else
    {
        //
        // The first statement in a nonempty block that is not a
        // switch block is reachable iff the block is reachable.
        // Every other statement S in a nonempty block that is not a
        // switch block is reachable iff the statement preceeding S
        // can complete normally.
        //
        AstStatement* statement = block_body -> Statement(0);
        statement -> is_reachable = block_body -> is_reachable;
        AstStatement* first_unreachable_statement =
            (AstStatement*) (statement -> is_reachable ? NULL : statement);
        ProcessStatement(statement);
        for (unsigned i = 1; i < block_body -> NumStatements(); i++)
        {
            AstStatement* previous_statement = statement;
            statement = block_body -> Statement(i);
            statement -> is_reachable =
                previous_statement -> can_complete_normally;
            if (! statement -> is_reachable &&
                first_unreachable_statement == NULL)
            {
                first_unreachable_statement = statement;
            }
            ProcessStatement(statement);
        }

        if (statement -> can_complete_normally)
            block_body -> can_complete_normally = true;

        //
        // If we have one or more unreachable statements that are contained in
        // a reachable block then issue message. (If the enclosing block is
        // not reachable the message will be issued later for the enclosing
        // block.)
        //
        if (first_unreachable_statement &&
            LocalBlockStack().TopBlock() -> is_reachable)
        {
            if (first_unreachable_statement == statement)
            {
                ReportSemError(SemanticError::UNREACHABLE_STATEMENT,
                               statement);
            }
            else
            {
                ReportSemError(SemanticError::UNREACHABLE_STATEMENTS,
                               first_unreachable_statement -> LeftToken(),
                               statement -> RightToken());
            }
        }

        //
        // If an enclosed block has a higher max_variable_index than the
        // current block, update max_variable_index in the current_block,
        // accordingly.
        //
        BlockSymbol* block = block_body -> block_symbol;
        if (block -> max_variable_index <
            LocalBlockStack().TopMaxEnclosedVariableIndex())
        {
            block -> max_variable_index =
                LocalBlockStack().TopMaxEnclosedVariableIndex();
        }
    }
}


void Semantic::ProcessBlock(Ast* stmt)
{
    AstBlock* block_body = (AstBlock*) stmt;

    AstBlock* enclosing_block = LocalBlockStack().TopBlock();

    //
    // Guess that the number of elements will not exceed the number of
    // statements + 3. The +3 takes into account one label + one ForInit
    // declaration and one extra something else.
    //
    int table_size = block_body -> NumStatements() + 3;
    BlockSymbol* block =
        LocalSymbolTable().Top() -> InsertBlockSymbol(table_size);
    //
    // enclosing_block is not present only when we are processing the block
    // of a static initializer
    //
    block -> max_variable_index =
        enclosing_block
        ? enclosing_block -> block_symbol -> max_variable_index : 1;
    LocalSymbolTable().Push(block -> Table());

    block_body -> block_symbol = block;
    block_body -> nesting_level = LocalBlockStack().Size();
    LocalBlockStack().Push(block_body);

    //
    // Note that in constructing the Ast, the parser encloses each
    // labeled statement in its own block. Therefore the declaration
    // of this label will not conflict with the declaration of another
    // label with the same name declared at the same nesting level.
    //
    // For example, the following sequence of statements is legal:
    //
    //     l: a = b;
    //     l: b = c;
    //
    if (block_body -> label_opt != BAD_TOKEN)
    {
        NameSymbol* name_symbol =
            lex_stream -> NameSymbol(block_body -> label_opt);
        if (LocalSymbolTable().FindLabelSymbol(name_symbol))
        {
            ReportSemError(SemanticError::DUPLICATE_LABEL,
                           block_body -> label_opt,
                           name_symbol -> Name());
        }
        else
        {
            LabelSymbol* label =
                LocalSymbolTable().Top() -> InsertLabelSymbol(name_symbol);
            label -> block = block_body;
            label -> nesting_level = block_body -> nesting_level;
        }
    }

    ProcessBlockStatements(block_body);

    LocalBlockStack().Pop();
    LocalSymbolTable().Pop();

    //
    // Update the information for the block that immediately encloses the
    // current block.
    //
    if (enclosing_block && (LocalBlockStack().TopMaxEnclosedVariableIndex() <
                            block -> max_variable_index))
    {
        LocalBlockStack().TopMaxEnclosedVariableIndex() =
            block -> max_variable_index;
    }

    block -> CompressSpace(); // space optimization
}


void Semantic::WarnOfAccessibleFieldWithName(SemanticError::SemanticErrorKind problem,
                                             AstVariableDeclaratorId* name,
                                             NameSymbol* name_symbol,
                                             bool is_static)
{
    TypeSymbol* this_type = ThisType();
    for (TypeSymbol* type = this_type; type != 0; type = type -> super)
    {
        //
        // Try to find a variable with the same name_symbol, first in the
        // type itself, then in any of its implemented interfaces.
        //
        VariableSymbol* variable = type -> FindVariableSymbol(name_symbol);
        for (unsigned i = 0; variable == 0 && i < type -> NumInterfaces(); ++i)
        {
            variable = type -> Interface(i) -> FindVariableSymbol(name_symbol);
        }

        //
        // Warn if we found an accessible field with the same name_symbol.
        //
        if (variable && MemberAccessCheck(this_type, variable, 0))
        {
            // Ignore static variables if we're looking for non-static,
            // and non-static if we're looking for static.
            if (variable -> ACC_STATIC() != is_static)
            {
                continue;
            }

            TypeSymbol* containing_type = variable -> ContainingType();
            ReportSemError(problem,
                           name -> identifier_token,
                           name_symbol -> Name(),
                           containing_type -> ContainingPackageName(),
                           containing_type -> ExternalName());
            return;
        }
    }
}


void Semantic::ProcessLocalVariableStatement(Ast* stmt)
{
    AstLocalVariableStatement* local_decl = (AstLocalVariableStatement*) stmt;
    ProcessType(local_decl -> type);
    TypeSymbol* field_type = local_decl -> type -> symbol;
    AccessFlags access_flags = ProcessLocalModifiers(local_decl);

    for (unsigned i = 0; i < local_decl -> NumVariableDeclarators(); i++)
    {
        AstVariableDeclarator* variable_declarator =
            local_decl -> VariableDeclarator(i);
        AstVariableDeclaratorId* name =
            variable_declarator -> variable_declarator_name;
        NameSymbol* name_symbol =
            lex_stream -> NameSymbol(name -> identifier_token);

        //
        // According to JLS2 14.4.2, only check for a duplicate in
        // the local class scope; don't worry about enclosing classes
        //
        VariableSymbol* duplicate =
            LocalSymbolTable().FindVariableSymbol(name_symbol);
        if (duplicate)
        {
            ReportSemError(SemanticError::DUPLICATE_LOCAL_VARIABLE_DECLARATION,
                           name -> identifier_token, name_symbol -> Name(),
                           duplicate -> FileLoc());
        }
        else
        {
            WarnOfAccessibleFieldWithName(SemanticError::LOCAL_SHADOWS_FIELD,
                                          name, name_symbol, ThisMethod() -> ACC_STATIC());
            AstBlock* block = LocalBlockStack().TopBlock();
            SymbolTable* table;
            if (block -> Tag() == AstBlock::SWITCH)
            {
                //
                // Local variables declared in a switch statement are in scope
                // for the entire switch, rather than the nearest label (unlike
                // local classes).  Hence, we have to check if the top block is
                // a switch statement, and use the next level up if so.
                //
                block = LocalBlockStack()[LocalBlockStack().Size() - 2];
                table = LocalSymbolTable()[LocalSymbolTable().Size() - 2];
            }
            else table = LocalSymbolTable().Top();
            VariableSymbol* symbol =
                table -> InsertVariableSymbol(name_symbol);
            variable_declarator -> symbol = symbol;

            unsigned dims =
                field_type -> num_dimensions + name -> NumBrackets();
            symbol -> SetType(field_type -> GetArrayType(this, dims));
            symbol -> SetFlags(access_flags);
            symbol -> SetOwner(ThisMethod());
            symbol -> declarator = variable_declarator;

            // Copy parameterized type information if present
            AstTypeName* type_name = local_decl -> type -> TypeNameCast();
            if (type_name && type_name -> parameterized_type)
            {
                symbol -> parameterized_type = type_name -> parameterized_type;
            }

            symbol -> SetLocation();
            symbol -> SetLocalVariableIndex(block -> block_symbol ->
                                            max_variable_index++);
            if (control.IsDoubleWordType(symbol -> Type()))
                block -> block_symbol -> max_variable_index++;

            //
            // Warn against unconventional names. Note that there's no
            // strong convention for final local variables, so we allow
            // both the usual style for local variables and the usual
            // style for constant fields. We recommend the local variable
            // style, somewhat arbitrarily.
            //
            if (! symbol -> ACC_FINAL() &&
                name_symbol -> IsBadStyleForVariable())
            {
                ReportSemError(SemanticError::UNCONVENTIONAL_VARIABLE_NAME,
                               name -> identifier_token, name_symbol -> Name());
            }
            else if (symbol -> ACC_FINAL() &&
                     name_symbol -> IsBadStyleForVariable() &&
                     name_symbol -> IsBadStyleForConstantField())
            {
                ReportSemError(SemanticError::UNCONVENTIONAL_VARIABLE_NAME,
                               name -> identifier_token, name_symbol -> Name());
            }
            
            ProcessVariableInitializer(variable_declarator);
        }
    }

    //
    // A local variable declaration statement can complete normally
    // iff it is reachable.
    //
    local_decl -> can_complete_normally = local_decl -> is_reachable;
}


void Semantic::ProcessExpressionStatement(Ast* stmt)
{
    AstExpressionStatement* expression_statement =
        (AstExpressionStatement*) stmt;

    ProcessExpression(expression_statement -> expression);

    //
    // An expression statement can complete normally iff it is reachable.
    //
    expression_statement -> can_complete_normally =
        expression_statement -> is_reachable;
}


void Semantic::ProcessSynchronizedStatement(Ast* stmt)
{
    AstSynchronizedStatement* synchronized_statement =
        (AstSynchronizedStatement*) stmt;

    //
    // Notice that in the case of a complex string constant, it is
    // vital to inline correctly; otherwise, we would not be using
    // the correct object as the monitor.
    //
    ProcessExpressionOrStringConstant(synchronized_statement -> expression);

    synchronized_statement -> block -> is_reachable =
        synchronized_statement -> is_reachable;

    if (synchronized_statement -> expression -> Type() -> Primitive() ||
        synchronized_statement -> expression -> symbol == control.null_type)
    {
        ReportSemError(SemanticError::TYPE_NOT_REFERENCE,
                       synchronized_statement -> expression,
                       synchronized_statement -> expression -> Type() -> Name());
    }

    AstBlock* enclosing_block = LocalBlockStack().TopBlock();
    AstBlock* block_body = synchronized_statement -> block;

    //
    // Synchronized blocks require one special local variable slot for the
    // monitor. However, since a try-finally may require up to four slots, we
    // reserve them all at this time.  Otherwise, the sequence {synchronized;
    // variable declaration; try-finally} within the same enclosing block will
    // cause a VerifyError. The VM should not care if some of these special
    // slots are unused.
    //
    // TODO: Is it worth optimizing this and try-finally to avoid wasting
    // variable slots?
    //
    BlockSymbol* enclosing_block_symbol = enclosing_block -> block_symbol;
    // first such statement encountered in enclosing block?
    if (enclosing_block_symbol -> helper_variable_index < 0)
    {
        enclosing_block_symbol -> helper_variable_index =
            enclosing_block_symbol -> max_variable_index;
        enclosing_block_symbol -> max_variable_index += 2;
        if (ThisMethod() -> Type() != control.void_type)
        {
            if (control.IsDoubleWordType(ThisMethod() -> Type()))
                enclosing_block_symbol -> max_variable_index += 2;
            else enclosing_block_symbol -> max_variable_index += 1;
        }
    }

    //
    // Guess that the number of elements will not exceed the number of
    // statements + 3.
    //
    BlockSymbol* block = LocalSymbolTable().Top() ->
        InsertBlockSymbol(block_body -> NumStatements() + 3);
    block -> max_variable_index = enclosing_block_symbol -> max_variable_index;
    LocalSymbolTable().Push(block -> Table());

    block_body -> block_symbol = block;
    block_body -> nesting_level = LocalBlockStack().Size();
    LocalBlockStack().Push(block_body);

    ProcessBlockStatements(block_body);

    LocalBlockStack().Pop();
    LocalSymbolTable().Pop();

    if (LocalBlockStack().TopMaxEnclosedVariableIndex() <
        block -> max_variable_index)
    {
        LocalBlockStack().TopMaxEnclosedVariableIndex() =
            block -> max_variable_index;
    }

    synchronized_statement -> can_complete_normally =
        synchronized_statement -> block -> can_complete_normally;

    block -> CompressSpace(); // space optimization
}


void Semantic::CheckForAssignmentUsedAsTruthValue(Ast* expression)
{
    //
    // Warn about boolean assignments within if/while guards, i.e.
    // code such as "if (booleanLocal = booleanMethod())"
    // instead of "if (booleanLocal == booleanMethod())".
    //
    // We deliberately don't do anything like StripNops because
    // we want to allow the same compiler-quitening fix as gcc:
    // "if ((booleanLocal = booleanMethod()))".
    //
    AstAssignmentExpression* assignment_expression =
        expression -> AssignmentExpressionCast();
    if (assignment_expression &&
        assignment_expression -> SimpleAssignment() &&
        assignment_expression -> Type() == control.boolean_type)
    {
        ReportSemError(SemanticError::ASSIGNMENT_USED_AS_TRUTH_VALUE,
                       expression);
    }
}


void Semantic::ProcessIfStatement(Ast* stmt)
{
    AstIfStatement* if_statement = (AstIfStatement*) stmt;

    ProcessExpression(if_statement -> expression);

    TypeSymbol* cond_type = if_statement -> expression -> Type();
    // Allow Boolean wrapper type with auto-unboxing in Java 5+
    bool is_boolean = cond_type == control.boolean_type ||
                      cond_type == control.no_type ||
                      (control.option.source >= JopaOption::SDK1_5 &&
                       cond_type == control.Boolean());
    if (! is_boolean)
    {
        ReportSemError(SemanticError::TYPE_NOT_BOOLEAN,
                       if_statement -> expression,
                       cond_type -> ContainingPackageName(),
                       cond_type -> ExternalName());
    }
    CheckForAssignmentUsedAsTruthValue(if_statement -> expression);

    //
    // Recall that the parser enclosed both true and false statements in
    // blocks, if necessary.
    //
    if_statement -> true_statement -> is_reachable =
        if_statement -> is_reachable;
    ProcessBlock(if_statement -> true_statement);

    if (if_statement -> false_statement_opt)
    {
        if_statement -> false_statement_opt -> is_reachable =
            if_statement -> is_reachable;
        ProcessBlock(if_statement -> false_statement_opt);

        if_statement -> can_complete_normally =
            if_statement -> true_statement -> can_complete_normally ||
            if_statement -> false_statement_opt -> can_complete_normally;
    }
    else if_statement -> can_complete_normally = if_statement -> is_reachable;
}


void Semantic::ProcessWhileStatement(Ast* stmt)
{
    AstWhileStatement* while_statement = (AstWhileStatement*) stmt;

    //
    // Recall that each while statement is enclosed in a unique block by the
    // parser, as is the loop body.
    //
    BreakableStatementStack().Push(LocalBlockStack().TopBlock());
    ContinuableStatementStack().Push(LocalBlockStack().TopBlock());

    AstBlock* enclosed_statement = while_statement -> statement;
    enclosed_statement -> is_reachable = while_statement -> is_reachable;

    ProcessExpression(while_statement -> expression);
    TypeSymbol* cond_type = while_statement -> expression -> Type();
    // Allow Boolean wrapper type with auto-unboxing in Java 5+
    bool is_boolean_wrapper = control.option.source >= JopaOption::SDK1_5 &&
                              cond_type == control.Boolean();
    if (cond_type == control.boolean_type)
    {
        if (IsConstantFalse(while_statement -> expression))
        {
            if (while_statement -> is_reachable)
                while_statement -> can_complete_normally = true;
            enclosed_statement -> is_reachable = false;
        }
        else if (! IsConstantTrue(while_statement -> expression) &&
                 while_statement -> is_reachable)
        {
            while_statement -> can_complete_normally = true;
        }
    }
    else if (is_boolean_wrapper)
    {
        // Boolean wrapper can complete normally (value not known at compile time)
        if (while_statement -> is_reachable)
            while_statement -> can_complete_normally = true;
    }
    else if (cond_type != control.no_type)
    {
        ReportSemError(SemanticError::TYPE_NOT_BOOLEAN,
                       while_statement -> expression,
                       cond_type -> ContainingPackageName(),
                       cond_type -> ExternalName());
    }
    CheckForAssignmentUsedAsTruthValue(while_statement -> expression);

    ProcessBlock(enclosed_statement);

    if (! enclosed_statement -> is_reachable &&
        while_statement -> is_reachable)
    {
        ReportSemError(SemanticError::UNREACHABLE_STATEMENT,
                       enclosed_statement);
    }

    //
    // If the while statement contained a reachable break statement,
    // then the while statement can complete normally. It is marked
    // here only for completeness, as marking the enclosing block is
    // enough to propagate the proper information upward.
    //
    AstBlock* block_body = BreakableStatementStack().Top();
    if (block_body -> can_complete_normally)
        while_statement -> can_complete_normally = true;

    BreakableStatementStack().Pop();
    ContinuableStatementStack().Pop();
}


void Semantic::ProcessForStatement(Ast* stmt)
{
    AstForStatement* for_statement = (AstForStatement*) stmt;

    //
    // Note that in constructing the Ast, the parser encloses each
    // for-statement whose for-init-statements starts with a local
    // variable declaration in its own block. Therefore a redeclaration
    // of another local variable with the same name in a different loop
    // at the same nesting level will not cause any conflict.
    //
    // For example, the following sequence of statements is legal:
    //
    //     for (int i = 0; i < 10; i++);
    //     for (int i = 10; i < 20; i++);
    //
    for (unsigned i = 0; i < for_statement -> NumForInitStatements(); i++)
        ProcessStatement(for_statement -> ForInitStatement(i));

    //
    // Recall that each for statement is enclosed in a unique block by the
    // parser, as is the loop body.
    //
    BreakableStatementStack().Push(LocalBlockStack().TopBlock());
    ContinuableStatementStack().Push(LocalBlockStack().TopBlock());

    //
    // Assume that if the for_statement is reachable then its
    // contained statement is also reachable. If it turns out that the
    // condition (end) expression is a constant FALSE expression we will
    // change the assumption...
    //
    AstBlock* enclosed_statement = for_statement -> statement;
    enclosed_statement -> is_reachable = for_statement -> is_reachable;

    if (for_statement -> end_expression_opt)
    {
        ProcessExpression(for_statement -> end_expression_opt);
        TypeSymbol* cond_type = for_statement -> end_expression_opt -> Type();
        // Allow Boolean wrapper type with auto-unboxing in Java 5+
        bool is_boolean_wrapper = control.option.source >= JopaOption::SDK1_5 &&
                                  cond_type == control.Boolean();
        if (cond_type == control.boolean_type)
        {
            if (IsConstantFalse(for_statement -> end_expression_opt))
            {
                if (for_statement -> is_reachable)
                    for_statement -> can_complete_normally = true;
                enclosed_statement -> is_reachable = false;
            }
            else if (! IsConstantTrue(for_statement -> end_expression_opt) &&
                     for_statement -> is_reachable)
            {
                for_statement -> can_complete_normally = true;
            }
        }
        else if (is_boolean_wrapper)
        {
            // Boolean wrapper can complete normally (value not known at compile time)
            if (for_statement -> is_reachable)
                for_statement -> can_complete_normally = true;
        }
        else if (cond_type != control.no_type)
        {
            ReportSemError(SemanticError::TYPE_NOT_BOOLEAN,
                           for_statement -> end_expression_opt,
                           cond_type -> ContainingPackageName(),
                           cond_type -> ExternalName());
        }
    }

    ProcessBlock(enclosed_statement);

    if (! enclosed_statement -> is_reachable &&
        for_statement -> is_reachable)
    {
        ReportSemError(SemanticError::UNREACHABLE_STATEMENT,
                       enclosed_statement);
    }

    for (unsigned j = 0; j < for_statement -> NumForUpdateStatements(); j++)
        ProcessExpressionStatement(for_statement -> ForUpdateStatement(j));

    //
    // If the for statement contained a reachable break statement,
    // then the for statement can complete normally. It is marked
    // here only for completeness, as marking the enclosing block is
    // enough to propagate the proper information upward.
    //
    AstBlock* block_body = BreakableStatementStack().Top();
    if (block_body -> can_complete_normally)
        for_statement -> can_complete_normally = true;

    BreakableStatementStack().Pop();
    ContinuableStatementStack().Pop();
}


//
// Enhanced for loops (or foreach loops) were added in JDK 1.5, by JSR 201.
//
struct SearchNode {
     TypeSymbol* type_sym;
     ParameterizedType* param_type;
};

TypeSymbol* Semantic::GetIterableElementType(TypeSymbol* type, ParameterizedType* start_param)
{
    // Default to Object
    TypeSymbol* component_type = control.Object();
    
    if (!type) return component_type;

    // Check for parameterized Iterable type (e.g., Iterable<Integer>)
    ParameterizedType* param_iter = NULL;

    // First check if type itself is a parameterized Iterable
    // (this happens if type IS Iterable<T>, not a subtype)
    if (type -> GetParameterizedType() &&
        type -> GetParameterizedType() -> generic_type == control.Iterable())
    {
        param_iter = type -> GetParameterizedType();
    }
    else
    {
        // If start_param wasn't provided, try to get it from the type
        if (!start_param && type->GetParameterizedType() && 
            type->GetParameterizedType()->generic_type == type) {
             start_param = type->GetParameterizedType();
        }

        SymbolSet visited_types;
        Tuple<SearchNode> queue(16);
        
        SearchNode start_node;
        start_node.type_sym = type;
        start_node.param_type = start_param;
        
        queue.Next() = start_node;
        visited_types.AddElement(type);
        
        int depth = 0;
        for (unsigned i = 0; i < queue.Length() && !param_iter && depth < 50000; i++)
        {
            SearchNode current = queue[i];
            TypeSymbol* t = current.type_sym;
            ParameterizedType* p = current.param_type;
            depth++;
            
            // Check if t itself is Iterable
            if (t == control.Iterable()) {
                param_iter = p ? p : (ParameterizedType*)1;
                break;
            }

            // Check interfaces
            for (unsigned k = 0; k < t -> NumInterfaces(); k++)
            {
                TypeSymbol* interf = t -> Interface(k);
                ParameterizedType* decl_param = t -> ParameterizedInterface(k);
                ParameterizedType* next_param = decl_param;

                // Perform substitution if we have a context (p) and the interface is parameterized
                if (decl_param && p) {
                    unsigned num_args = decl_param->NumTypeArguments();
                    Tuple<Type*>* new_args = new Tuple<Type*>(num_args);
                    for(unsigned a=0; a<num_args; a++) {
                        Type* arg = decl_param->TypeArgument(a);
                        Type* subst_arg = arg; // Default to original

                        if (arg->kind == Type::TYPE_PARAMETER) {
                            unsigned pos = arg->type_parameter->position;
                            if (pos < p->NumTypeArguments()) {
                                // Substitute with argument from context
                                subst_arg = new Type(*p->TypeArgument(pos));
                            } else {
                                 subst_arg = new Type(*arg);
                            }
                        } else {
                            // Clone other types to avoid ownership issues if needed
                            // For now, simple clone
                            subst_arg = new Type(*arg);
                        }
                        new_args->Next() = subst_arg;
                    }
                    next_param = new ParameterizedType(decl_param->generic_type, new_args);
                    control.RegisterParameterizedType(next_param);
                }

                if (interf == control.Iterable())
                {
                    param_iter = next_param ? next_param : (ParameterizedType*)1;
                    break;
                }
                
                // Add interface to queue if not visited
                if (!visited_types.IsElement(interf))
                {
                    visited_types.AddElement(interf);
                    SearchNode next_node;
                    next_node.type_sym = interf;
                    next_node.param_type = next_param;
                    queue.Next() = next_node;
                }
            }
            
            if (param_iter) break;
            
            // Add superclass to queue if not visited
            if (t -> super && !visited_types.IsElement(t -> super))
            {
                visited_types.AddElement(t -> super);
                
                SearchNode next_node;
                next_node.type_sym = t -> super;
                next_node.param_type = p; // Pass through
                
                queue.Next() = next_node;
            }
        }
    }

    if (param_iter && param_iter != (ParameterizedType*)1 && param_iter -> NumTypeArguments() > 0)
    {
        Type* arg = param_iter -> TypeArgument(0);
        // Resolve the type argument (erasure + generics info)
        TypeSymbol* type_arg = arg -> Erasure(control);
        if (type_arg)
        {
            component_type = type_arg;
        }
    }
    return component_type;
}

//
// Enhanced for loops (or foreach loops) were added in JDK 1.5, by JSR 201.
//
void Semantic::ProcessForeachStatement(Ast* stmt)
{
    AstForeachStatement* foreach = (AstForeachStatement*) stmt;

    //
    // Note that in constructing the Ast, the parser encloses each
    // for-statement whose for-init-statements starts with a local
    // variable declaration in its own block. Therefore a redeclaration
    // of another local variable with the same name in a different loop
    // at the same nesting level will not cause any conflict.
    //
    // For example, the following sequence of statements is legal:
    //
    //     for (int i : new int[0]);
    //     for (int i : new int[0]);
    //
    // Recall that each for statement is enclosed in a unique block by the
    // parser, as is the loop body.
    //
    AstBlock* enclosing_block = LocalBlockStack().TopBlock();
    BlockSymbol* enclosing_block_symbol = enclosing_block -> block_symbol;
    assert(enclosing_block_symbol -> helper_variable_index < 0);
    BreakableStatementStack().Push(enclosing_block);
    ContinuableStatementStack().Push(enclosing_block);

    //
    // The contained statement of a foreach is reachable iff the foreach is
    // reachable.
    //
    AstBlock* enclosed_statement = foreach -> statement;
    enclosed_statement -> is_reachable = foreach -> is_reachable;

    ProcessType(foreach -> formal_parameter -> type);
    assert(! foreach -> formal_parameter -> ellipsis_token_opt);
    TypeSymbol* index_type = foreach -> formal_parameter -> type -> symbol;
    AccessFlags access_flags =
        ProcessFormalModifiers(foreach -> formal_parameter);
    AstVariableDeclarator* variable_declarator =
        foreach -> formal_parameter -> formal_declarator;
    AstVariableDeclaratorId* name =
        variable_declarator -> variable_declarator_name;
    NameSymbol* name_symbol =
        lex_stream -> NameSymbol(name -> identifier_token);
    VariableSymbol* duplicate =
        LocalSymbolTable().FindVariableSymbol(name_symbol);
    if (duplicate)
    {
        ReportSemError(SemanticError::DUPLICATE_LOCAL_VARIABLE_DECLARATION,
                       name -> identifier_token, name_symbol -> Name(),
                       duplicate -> FileLoc());
    }
    else
    {
        WarnOfAccessibleFieldWithName(SemanticError::LOCAL_SHADOWS_FIELD,
                                      name, name_symbol, false);
        SymbolTable* table = LocalSymbolTable().Top();
        VariableSymbol* symbol = table -> InsertVariableSymbol(name_symbol);
        variable_declarator -> symbol = symbol;
        unsigned dims = index_type -> num_dimensions + name -> NumBrackets();
        symbol -> SetType(index_type -> GetArrayType(this, dims));
        symbol -> SetFlags(access_flags);
        symbol -> SetOwner(ThisMethod());
        symbol -> declarator = variable_declarator;

        // Copy parameterized type information if present (for generics type substitution)
        AstTypeName* type_name = foreach -> formal_parameter -> type -> TypeNameCast();
        if (type_name && type_name -> parameterized_type)
        {
            symbol -> parameterized_type = type_name -> parameterized_type;
        }

        symbol -> SetLocation();
        symbol -> SetLocalVariableIndex(enclosing_block_symbol ->
                                        max_variable_index++);
        if (control.IsDoubleWordType(symbol -> Type()))
            enclosing_block_symbol -> max_variable_index++;

        //
        // Warn against unconventional names. Note that there's no
        // strong convention for final local variables, so we allow
        // both the usual style for local variables and the usual
        // style for constant fields. We recommend the local variable
        // style, somewhat arbitrarily.
        //
        if (! symbol -> ACC_FINAL() && name_symbol -> IsBadStyleForVariable())
        {
            ReportSemError(SemanticError::UNCONVENTIONAL_VARIABLE_NAME,
                           name -> identifier_token, name_symbol -> Name());
        }
        else if (symbol -> ACC_FINAL() &&
                 name_symbol -> IsBadStyleForVariable() &&
                 name_symbol -> IsBadStyleForConstantField())
        {
            ReportSemError(SemanticError::UNCONVENTIONAL_VARIABLE_NAME,
                           name -> identifier_token, name_symbol -> Name());
        }
    }

    ProcessExpression(foreach -> expression);
    TypeSymbol* cond_type = foreach -> expression -> Type();
    TypeSymbol* component_type;
    if (control.option.source < JopaOption::SDK1_5)
    {
        ReportSemError(SemanticError::FOREACH_UNSUPPORTED,
                       stmt -> RightToken(),
                       foreach -> statement -> LeftToken() - 1);
    }
    else if (cond_type -> IsArray())
    {
        component_type = cond_type -> ArraySubtype();
        if (! CanAssignmentConvertReference(index_type, component_type))
        {
            ReportSemError(SemanticError::INCOMPATIBLE_TYPE_FOR_FOREACH,
                           foreach -> expression,
                           component_type -> ContainingPackageName(),
                           component_type -> ExternalName(),
                           index_type -> ContainingPackageName(),
                           index_type -> ExternalName());
        }
        // Need local variabls to stash array, array.length, and counter.
        enclosing_block_symbol -> helper_variable_index =
            enclosing_block_symbol -> max_variable_index;
        enclosing_block_symbol -> max_variable_index += 3;
    }
    else if (cond_type -> IsSubtype(control.Iterable()))
    {
        component_type = GetIterableElementType(cond_type, foreach->expression->resolved_parameterized_type);

        if (! CanAssignmentConvertReference(index_type, component_type))
        {
            ReportSemError(SemanticError::INCOMPATIBLE_TYPE_FOR_FOREACH,
                           foreach -> expression,
                           component_type -> ContainingPackageName(),
                           component_type -> ExternalName(),
                           index_type -> ContainingPackageName(),
                           index_type -> ExternalName());
        }
        // Need synthetic local variable to stash iterator.
        enclosing_block_symbol -> helper_variable_index =
            enclosing_block_symbol -> max_variable_index;
        enclosing_block_symbol -> max_variable_index++;
    }
    else if (cond_type != control.no_type)
    {
        ReportSemError(SemanticError::TYPE_NOT_ITERABLE, foreach -> expression,
                       cond_type -> ContainingPackageName(),
                       cond_type -> ExternalName());
    }
    ProcessBlock(enclosed_statement);

    //
    // Foreach statements can always complete normally, if reachable, because
    // the array/iterator length could be 0.
    //
    foreach -> can_complete_normally = foreach -> is_reachable;
    BreakableStatementStack().Pop();
    ContinuableStatementStack().Pop();
}


void Semantic::ProcessSwitchStatement(Ast* stmt)
{
    AstSwitchStatement* switch_statement = (AstSwitchStatement*) stmt;

    AstBlock* enclosing_block = LocalBlockStack().TopBlock();

    //
    // We estimate a size for the switch symbol table based on the number of
    // lines in it. In a switch statement, local variable declarations have
    // scope over the entire main_block, but local classes only have scope in
    // the current switch block statement.
    //
    AstBlock* block_body = switch_statement -> switch_block;
    BlockSymbol* main_block = LocalSymbolTable().Top() -> InsertBlockSymbol();
    main_block -> max_variable_index =
        enclosing_block -> block_symbol -> max_variable_index;
    LocalSymbolTable().Push(main_block -> Table());

    block_body -> block_symbol = main_block;
    block_body -> nesting_level = LocalBlockStack().Size();
    LocalBlockStack().Push(block_body);
    BreakableStatementStack().Push(block_body);

    ProcessExpression(switch_statement -> expression);
    TypeSymbol* type = switch_statement -> expression -> Type();

    bool is_string_switch = (type == control.String());
    switch_statement -> is_string_switch = is_string_switch;

    bool is_enum_switch = (type && (type -> ACC_ENUM() || type -> IsEnum()));
    switch_statement -> is_enum_switch = is_enum_switch;
    switch_statement -> enum_type = is_enum_switch ? type : NULL;

    if (is_string_switch)
    {
        if (main_block -> helper_variable_index < 0)
        {
            main_block -> helper_variable_index =
                main_block -> max_variable_index;
            main_block -> max_variable_index++;
        }
    }

    if (! control.IsSimpleIntegerValueType(type) &&
        type != control.String() &&
        ! is_enum_switch &&
        type != control.no_type)
    {
        ReportSemError(SemanticError::TYPE_NOT_INTEGER,
                       switch_statement -> expression,
                       type -> ContainingPackageName(),
                       type -> ExternalName());
        type = control.no_type;
    }

    //
    // Count the number of case labels in this switch statement.
    //
    unsigned num_case_labels = 0;
    for (unsigned i = 0; i < block_body -> NumStatements(); i++)
        num_case_labels += switch_statement -> Block(i) -> NumSwitchLabels();
    switch_statement -> AllocateCases(num_case_labels);

    //
    // A switch block is reachable iff its switch statement is reachable.
    //
    block_body -> is_reachable = switch_statement -> is_reachable;
    for (unsigned j = 0; j < block_body -> NumStatements(); j++)
    {
        AstSwitchBlockStatement* switch_block_statement =
            switch_statement -> Block(j);
        for (unsigned k = 0;
             k < switch_block_statement -> NumSwitchLabels(); k++)
        {
            AstSwitchLabel* switch_label =
                switch_block_statement -> SwitchLabel(k);
            if (switch_label -> expression_opt)
            {
                // For enum switch, case labels must be unqualified enum constant names.
                // Handle them specially without calling ProcessExpression.
                if (switch_statement -> is_enum_switch)
                {
                    AstName* name = switch_label -> expression_opt -> NameCast();
                    VariableSymbol* enum_constant = NULL;

                    if (name && ! name -> base_opt)
                    {
                        // Simple name - look it up in the enum type
                        NameSymbol* name_symbol = lex_stream -> NameSymbol(name -> identifier_token);
                        enum_constant =
                            switch_statement -> enum_type -> FindVariableSymbol(name_symbol);
                        if (enum_constant && ! enum_constant -> ACC_ENUM())
                            enum_constant = NULL; // Not an enum constant
                    }

                    if (! enum_constant)
                    {
                        // Not a valid enum constant - report error
                        ReportSemError(SemanticError::INVALID_ENUM_SWITCH_CASE,
                                       switch_label -> expression_opt,
                                       switch_statement -> enum_type -> ContainingPackageName(),
                                       switch_statement -> enum_type -> ExternalName());
                    }
                    else
                    {
                        // Valid enum constant - create case element
                        CaseElement* case_element =
                            compilation_unit -> ast_pool -> GenCaseElement(j, k);
                        switch_statement -> AddCase(case_element);
                        case_element -> value = enum_constant -> enum_ordinal;
                        case_element -> string_value = NULL;
                    }
                    continue; // Skip the rest of the case label processing
                }

                ProcessExpression(switch_label -> expression_opt);
                TypeSymbol* case_type =
                    switch_label -> expression_opt -> Type();
                if (case_type == control.no_type)
                    continue;

                if (is_string_switch)
                {
                    if (case_type != control.String())
                    {
                        ReportSemError(SemanticError::TYPE_NOT_STRING,
                                       switch_label -> expression_opt,
                                       case_type -> ContainingPackageName(),
                                       case_type -> ExternalName());
                        switch_label -> expression_opt -> symbol = control.no_type;
                    }
                    else if (! switch_label -> expression_opt -> IsConstant())
                    {
                        ReportSemError(SemanticError::EXPRESSION_NOT_CONSTANT,
                                       switch_label -> expression_opt);
                        switch_label -> expression_opt -> symbol = control.no_type;
                    }
                    else
                    {
                        Utf8LiteralValue* str_value = DYNAMIC_CAST<Utf8LiteralValue*>
                            (switch_label -> expression_opt -> value);
                        CaseElement* case_element =
                            compilation_unit -> ast_pool -> GenCaseElement(j, k);
                        switch_statement -> AddCase(case_element);
                        i4 hash_code = 0;
                        for (int i = 0; i < str_value -> length; i++)
                            hash_code = 31 * hash_code + (unsigned char)str_value -> value[i];
                        case_element -> value = hash_code;
                        case_element -> string_value = lex_stream ->
                            NameString(switch_label -> expression_opt -> LeftToken());
                    }
                }
                else if (! control.IsSimpleIntegerValueType(case_type))
                {
                    ReportSemError(SemanticError::TYPE_NOT_INTEGER,
                                   switch_label -> expression_opt,
                                   case_type -> ContainingPackageName(),
                                   case_type -> ExternalName());
                    switch_label -> expression_opt -> symbol = control.no_type;
                }
                else if (! switch_label -> expression_opt -> IsConstant())
                {
                    ReportSemError(SemanticError::EXPRESSION_NOT_CONSTANT,
                                   switch_label -> expression_opt);
                    switch_label -> expression_opt -> symbol = control.no_type;
                }
                else if (CanAssignmentConvert(type,
                                              switch_label -> expression_opt))
                {
                    switch_label -> expression_opt =
                        ConvertToType(switch_label -> expression_opt, type);
                    CaseElement* case_element =
                        compilation_unit -> ast_pool -> GenCaseElement(j, k);
                    switch_statement -> AddCase(case_element);
                    case_element -> value = DYNAMIC_CAST<IntLiteralValue*>
                        (switch_label -> expression_opt -> value) -> value;
                    case_element -> string_value = NULL;
                }
                else
                {
                    IntToWstring value(DYNAMIC_CAST<IntLiteralValue*>
                                       (switch_label -> expression_opt ->
                                        value) -> value);
                    ReportSemError(SemanticError::VALUE_NOT_REPRESENTABLE_IN_SWITCH_TYPE,
                                   switch_label -> expression_opt,
                                   value.String(),
                                   type -> Name());
                }
            }
            else if (! switch_statement -> DefaultCase())
            {
                switch_statement -> DefaultCase() =
                    compilation_unit -> ast_pool -> GenCaseElement(j, k);
                switch_label -> map_index = num_case_labels - 1;
            }
            else
            {
                ReportSemError(SemanticError::MULTIPLE_DEFAULT_LABEL,
                               switch_block_statement -> SwitchLabel(k));
            }
        }

        //
        // The parser ensures that a switch block statement always has one
        // statement. When a switch block ends with a sequence of switch
        // labels that are not followed by any executable statements, an
        // artificial "empty" statement is added by the parser. Another
        // BlockSymbol level is used here for the scope of local classes,
        // but we share the BlockStack level to make break work correctly.
        //
        assert(switch_block_statement -> NumStatements() > 0);
        BlockSymbol* statement_block =
            LocalSymbolTable().Top() -> InsertBlockSymbol();
        statement_block -> max_variable_index =
            main_block -> max_variable_index;
        LocalSymbolTable().Push(statement_block -> Table());

        switch_block_statement -> block_symbol = statement_block;
        switch_block_statement -> nesting_level = LocalBlockStack().Size();
        LocalBlockStack().Push(block_body);
        switch_block_statement -> is_reachable =
            switch_statement -> is_reachable;
        ProcessBlockStatements(switch_block_statement);
        if (switch_block_statement -> can_complete_normally &&
            j != block_body -> NumStatements() - 1)
        {
            //
            // TODO: Improve the parser to allow this warning to be locally
            // disabled by adding a comment similar to "// fallthrough".
            //
            ReportSemError(SemanticError::SWITCH_FALLTHROUGH,
                           switch_block_statement);
        }

        if (statement_block -> max_variable_index <
            LocalBlockStack().TopMaxEnclosedVariableIndex())
        {
            statement_block -> max_variable_index =
                LocalBlockStack().TopMaxEnclosedVariableIndex();
        }
        LocalBlockStack().Pop();
        LocalSymbolTable().Pop();
        if (LocalBlockStack().TopMaxEnclosedVariableIndex() <
            statement_block -> max_variable_index)
        {
            LocalBlockStack().TopMaxEnclosedVariableIndex() =
                statement_block -> max_variable_index;
        }
        statement_block -> CompressSpace();
    }

    //
    // A switch statement can complete normally iff at least one of the
    // following is true:
    //
    // . The last statement in the switch block can complete normally.
    // . The switch block is empty or contains only switch labels.
    // . There is at least one switch label after the last switch block
    //   statement group.
    // . The switch block does not contain a default label.
    // . There is a reachable break statement that exits the switch
    //   statement. (See ProcessBreakStatement)
    //
    if (block_body -> can_complete_normally ||
        ! switch_statement -> DefaultCase())
    {
        switch_statement -> can_complete_normally = true;
    }
    else
    {
        AstSwitchBlockStatement* last_switch_block_statement =
            switch_statement -> Block(block_body -> NumStatements() - 1);

        assert(last_switch_block_statement -> NumStatements() > 0);

        AstStatement* last_statement = last_switch_block_statement ->
            Statement(last_switch_block_statement -> NumStatements() - 1);
        if (last_statement -> can_complete_normally)
            switch_statement -> can_complete_normally = true;
    }

    //
    // Iterate over the sorted cases, checking for duplicates, and setting
    // the map_index field of each AstCaseLabel (1-based, in order to leave
    // room for the default label).
    //
    if (switch_statement -> NumCases())
    {
        switch_statement -> SortCases();
        CaseElement* first_case = switch_statement -> Case(0);
        switch_statement -> Block(first_case -> block_index) ->
            SwitchLabel(first_case -> case_index) -> map_index = 0;
    }
    for (unsigned k = 1; k < switch_statement -> NumCases(); k++)
    {
        CaseElement* case_elt = switch_statement -> Case(k);
        switch_statement -> Block(case_elt -> block_index) ->
            SwitchLabel(case_elt -> case_index) -> map_index = k;
        if (case_elt -> value == switch_statement -> Case(k - 1) -> value)
        {
            IntToWstring value(case_elt -> value);
            ReportSemError(SemanticError::DUPLICATE_CASE_VALUE,
                           (switch_statement ->
                            Block(case_elt -> block_index) ->
                            SwitchLabel(case_elt -> case_index) ->
                            expression_opt),
                           value.String());
        }
    }

    //
    // If an enclosed block has a higher max_variable_index than the current
    // block, update max_variable_index in the current_block, accordingly.
    // Also, update the information for the block that immediately encloses
    // the current block.
    //
    if (main_block -> max_variable_index <
        LocalBlockStack().TopMaxEnclosedVariableIndex())
    {
        main_block -> max_variable_index =
            LocalBlockStack().TopMaxEnclosedVariableIndex();
    }

    BreakableStatementStack().Pop();
    LocalBlockStack().Pop();
    LocalSymbolTable().Pop();

    if (enclosing_block && (LocalBlockStack().TopMaxEnclosedVariableIndex() <
                            main_block -> max_variable_index))
    {
        LocalBlockStack().TopMaxEnclosedVariableIndex() =
            main_block -> max_variable_index;
    }

    main_block -> CompressSpace(); // space optimization
}


void Semantic::ProcessDoStatement(Ast* stmt)
{
    AstDoStatement* do_statement = (AstDoStatement*) stmt;

    //
    // Recall that each Do statement is enclosed in a unique block by the
    // parser, as is the loop body.
    //
    BreakableStatementStack().Push(LocalBlockStack().TopBlock());
    ContinuableStatementStack().Push(LocalBlockStack().TopBlock());

    AstBlock* enclosed_statement = do_statement -> statement;
    enclosed_statement -> is_reachable = do_statement -> is_reachable;

    ProcessBlock(enclosed_statement);

    ProcessExpression(do_statement -> expression);

    TypeSymbol* type = do_statement -> expression -> Type();
    // Allow Boolean wrapper type with auto-unboxing in Java 5+
    bool is_boolean = type == control.boolean_type ||
                      type == control.no_type ||
                      (control.option.source >= JopaOption::SDK1_5 &&
                       type == control.Boolean());
    if (! is_boolean)
    {
        ReportSemError(SemanticError::TYPE_NOT_BOOLEAN,
                       do_statement -> expression,
                       type -> ContainingPackageName(),
                       type -> ExternalName());
    }
    CheckForAssignmentUsedAsTruthValue(do_statement -> expression);

    //
    // A do statement can complete normally, iff at least one of the following
    // is true:
    //     1. The contained statement can complete normally and the condition
    //        expression is not a constant expression with the value true
    //     2. There is a reachable break statement that exits the do statement
    //        (This condition is true if the block that immediately encloses
    //        this do statement can complete normally. See
    //        ProcessBreakStatement)
    //
    AstBlock* block_body = (AstBlock*) BreakableStatementStack().Top();
    do_statement -> can_complete_normally =
        ((enclosed_statement -> can_complete_normally &&
          ! IsConstantTrue(do_statement -> expression)) ||
         block_body -> can_complete_normally);

    BreakableStatementStack().Pop();
    ContinuableStatementStack().Pop();
}


void Semantic::ProcessBreakStatement(Ast* stmt)
{
    AstBreakStatement* break_statement = (AstBreakStatement*) stmt;

    //
    // Recall that it is possible to break out of any labeled statement even
    // if it is not a do, for, while or switch statement.
    //
    if (break_statement -> identifier_token_opt)
    {
        NameSymbol* name_symbol =
            lex_stream -> NameSymbol(break_statement -> identifier_token_opt);
        LabelSymbol* label_symbol =
            LocalSymbolTable().FindLabelSymbol(name_symbol);

        if (label_symbol)
        {
            break_statement -> nesting_level = label_symbol -> nesting_level;
            AstBlock* block_body = label_symbol -> block;
            //
            // A labeled statement can complete normally if there is a
            // reachable break statement that exits the labeled statement.
            // If the break occurs in a try or catch block with a finally
            // block that completes abruptly, the break is discarded.
            //
            if (block_body && break_statement -> is_reachable &&
                AbruptFinallyStack().Top() < block_body -> nesting_level)
            {
                block_body -> can_complete_normally = true;
            }
        }
        else
        {
            AstBlock* block_body = (AstBlock*) LocalBlockStack().TopBlock();
            break_statement -> nesting_level = block_body -> nesting_level;
            ReportSemError(SemanticError::UNDECLARED_LABEL,
                           break_statement -> identifier_token_opt,
                           lex_stream -> NameString(break_statement ->
                                                    identifier_token_opt));
        }
    }
    else
    {
        AstBlock* block_body =
            (AstBlock*) (BreakableStatementStack().Size() > 0
                          ? BreakableStatementStack().Top()
                          : LocalBlockStack().TopBlock());
        break_statement -> nesting_level = block_body -> nesting_level;
        if (BreakableStatementStack().Size() > 0)
        {
            if (break_statement -> is_reachable &&
                AbruptFinallyStack().Top() < block_body -> nesting_level)
            {
                block_body -> can_complete_normally = true;
            }
        }
        else ReportSemError(SemanticError::MISPLACED_BREAK_STATEMENT,
                            break_statement);
    }
}


void Semantic::ProcessContinueStatement(Ast* stmt)
{
    AstContinueStatement* continue_statement = (AstContinueStatement*) stmt;

    //
    // The loop statement that is to be continued.
    //
    Ast* loop_statement = NULL;

    if (ContinuableStatementStack().Size() <= 0)
    {
        ReportSemError(SemanticError::MISPLACED_CONTINUE_STATEMENT,
                       continue_statement);
    }
    else if (continue_statement -> identifier_token_opt)
    {
        NameSymbol* name_symbol = lex_stream ->
            NameSymbol(continue_statement -> identifier_token_opt);
        LabelSymbol* label_symbol =
            LocalSymbolTable().FindLabelSymbol(name_symbol);

        if (label_symbol)
        {
            continue_statement -> nesting_level =
                label_symbol -> nesting_level;

            assert(label_symbol -> block -> NumStatements() > 0);

            loop_statement = label_symbol -> block -> Statement(0);
        }
        else
        {
            AstBlock* block_body = (AstBlock*) LocalBlockStack().TopBlock();
            continue_statement -> nesting_level = block_body -> nesting_level;
            ReportSemError(SemanticError::UNDECLARED_LABEL,
                           continue_statement -> identifier_token_opt,
                           lex_stream -> NameString(continue_statement ->
                                                    identifier_token_opt));
        }
    }
    else
    {
        AstBlock* block_body = (AstBlock*) ContinuableStatementStack().Top();
        loop_statement = block_body -> Statement(0);
        continue_statement -> nesting_level = block_body -> nesting_level;
    }

    //
    // If this is a valid continue statement, it is associated with a loop
    // statement. The parser created a block, if necessary, so that the loop
    // body is always a block, and we mark it as "can complete normally".
    // However, if the continue occurs in a try or catch block with a
    // corresponding abrupt finally clause, the continue is discarded.
    //
    if (loop_statement)
    {
        AstDoStatement* do_statement = loop_statement -> DoStatementCast();
        AstForStatement* for_statement = loop_statement -> ForStatementCast();
        AstWhileStatement* while_statement =
            loop_statement -> WhileStatementCast();
        AstForeachStatement* foreach_statement =
            loop_statement -> ForeachStatementCast();

        AstBlock* enclosed_statement = (do_statement
                                        ? do_statement -> statement
                                        : for_statement
                                        ? for_statement -> statement
                                        : while_statement
                                        ? while_statement -> statement
                                        : foreach_statement
                                        ? foreach_statement -> statement
                                        : (AstBlock*) NULL);
        if (enclosed_statement)
        {
            if (AbruptFinallyStack().Top() <
                continue_statement -> nesting_level)
            {
                enclosed_statement -> can_complete_normally = true;
            }
        }
        else
        {
            assert(continue_statement -> identifier_token_opt);

            ReportSemError(SemanticError::INVALID_CONTINUE_TARGET,
                           continue_statement,
                           lex_stream -> NameString(continue_statement ->
                                                    identifier_token_opt));
        }
    }
}


void Semantic::ProcessReturnStatement(Ast* stmt)
{
    AstReturnStatement* return_statement = (AstReturnStatement*) stmt;
    MethodSymbol* this_method = ThisMethod();

    if (this_method -> name_symbol == control.clinit_name_symbol ||
        this_method -> name_symbol == control.block_init_name_symbol)
    {
        ReportSemError(SemanticError::RETURN_STATEMENT_IN_INITIALIZER,
                       return_statement);
    }
    else if (return_statement -> expression_opt)
    {
        AstExpression* expression = return_statement -> expression_opt;
        ProcessExpressionOrStringConstant(expression);
        TypeSymbol* method_type = this_method -> Type();
        TypeSymbol* expression_type = expression -> Type();

        if (method_type == control.void_type ||
            this_method -> name_symbol == control.init_name_symbol)
        {
            ReportSemError(SemanticError::MISPLACED_RETURN_WITH_EXPRESSION,
                           return_statement);
        }
        else if (expression_type == control.null_type &&
                 method_type -> IsArray())
        {
            ReportSemError(SemanticError::EJ_RETURN_OF_NULL_ARRAY,
                           return_statement);
        }
        else if (expression_type != control.no_type)
        {
            if (method_type != expression_type)
            {
                if (CanAssignmentConvert(method_type, expression))
                    return_statement -> expression_opt =
                        ConvertToType(expression, method_type);
                else
                {
                    ReportSemError(SemanticError::MISMATCHED_RETURN_AND_METHOD_TYPE,
                                   expression,
                                   expression_type -> ContainingPackageName(),
                                   expression_type -> ExternalName(),
                                   method_type -> ContainingPackageName(),
                                   method_type -> ExternalName());
                }
            }
        }
    }
    else if (this_method -> Type() != control.void_type &&
             this_method -> name_symbol != control.init_name_symbol)
    {
        ReportSemError(SemanticError::MISPLACED_RETURN_WITH_NO_EXPRESSION,
                       return_statement);
    }
}


//
// Any exception that is neither RuntimeException or one of its subclasses
// nor Error or one of its subclasses is a checked exception. This also
// ignores invalid types. Additionally, 'throw null' results in a
// NullPointerException, so it is unchecked.
//
bool Semantic::CheckedException(TypeSymbol* exception)
{
    return (exception != control.null_type &&
            exception != control.no_type &&
            ! exception -> IsSubclass(control.RuntimeException()) &&
            ! exception -> IsSubclass(control.Error()));
}


bool Semantic::UncaughtException(TypeSymbol* exception)
{
    //
    // An unchecked exception or a bad type is ok !!
    //
    if (! CheckedException(exception))
        return false;

    //
    // Firstly, check the stack of try statements to see if the exception in
    // question is catchable.
    //
    int stack_size = TryStatementStack().Size();
    for (int i = stack_size - 1; i >= 0; i--)
    {
        AstTryStatement* try_statement = TryStatementStack()[i];

        //
        // If a try statement contains a finally clause that can't complete
        // normally then the exception is discarded, hence it is considered
        // catchable. See Spec 11.3.
        //
        if (try_statement -> finally_clause_opt &&
            (! try_statement -> finally_clause_opt -> block ->
             can_complete_normally))
        {
            return false;
        }

        //
        // Check each catch clause in turn if we are in the try block.
        //
        if (try_statement -> processing_try_block)
            for (unsigned k = 0; k < try_statement -> NumCatchClauses(); k++)
            {
                AstCatchClause* clause = try_statement -> CatchClause(k);
                VariableSymbol* symbol = clause -> parameter_symbol;
                if (CanAssignmentConvertReference(symbol -> Type(), exception))
                    return false;
            }
    }

    //
    // Check if the current method declares this in the throws clause (note
    // that field initializers are not in a current method).
    //
    MethodSymbol* this_method = ThisMethod();
    if (this_method)
    {
        for (unsigned l = this_method -> NumThrows(); l > 0; )
        {
            --l;
            if (CanAssignmentConvertReference(this_method -> Throws(l),
                                              exception))
                return false;
        }
    }

    //
    // In the special case of instance field initializers, and instance
    // initializer blocks, check if all constructors declare the exception
    // in the throws clause.
    //
    if ((this_method &&
         this_method -> Identity() == control.block_init_name_symbol) ||
        (ThisVariable() && ! ThisVariable() -> ACC_STATIC()))
    {
        TypeSymbol* this_type = ThisType();
        MethodSymbol* ctor =
            this_type -> FindMethodSymbol(control.init_name_symbol);
        if (! this_type -> Anonymous())
        {
            for ( ; ctor; ctor = ctor -> next_method)
            {
                int num_throws = (int) ctor -> NumThrows();
                int k;
                for (k = num_throws - 1; k >= 0; k--)
                {
                    if (CanAssignmentConvertReference(ctor -> Throws(k),
                                                      exception))
                        break;
                }
                if (k < 0) // No hit was found in constructor.
                    break;
            }
            return ctor != NULL; // Did all constructors catch exception?
        }
        else
        {
            assert(ctor);
            int num_throws = (int) ctor -> NumThrows();
            int k = 0;
            for (k = num_throws - 1; k >= 0; k--)
            {
                if (CanAssignmentConvertReference(ctor -> Throws(k),
                                                  exception))
                    break;
            }
            //
            // Anonymous classes must generate the constructor to handle all
            // possible initialization exceptions; this is possible because
            // a class instance can only be created at one point, so the
            // exception can be caught in the enclosing class. If we don't
            // find the exception, we must add it.
            //
            if (k < 0)
                ctor -> AddThrows(exception);
            return false;
        }
    }

    return true; // Nothing can catch the exception.
}


const wchar_t* Semantic::UncaughtExceptionContext()
{
    ErrorString s;
    MethodSymbol* this_method = ThisMethod();
    if (this_method)
    {
        s << " must be enclosed in a try statement that catches the "
          << "exception, ";
        if (this_method -> Identity() == control.clinit_name_symbol)
        {
            s << "since static initializers cannot throw checked exceptions.";
        }
        else
        {
            if (this_method -> Identity() == control.block_init_name_symbol)
            {
                assert(! ThisType() -> Anonymous());
                s << "or else every constructor in this class";
            }
            else if (this_method -> Identity() == control.init_name_symbol)
                s << "or else this constructor";
            else s << "or else this method";
            s << " must be declared to throw the exception.";
        }
    }
    else if (ThisType() -> ACC_INTERFACE())
    {
        s << " must be wrapped in a helper class method which catches the "
          << "exception, since interface field initializers cannot throw "
          << "checked exceptions.";
    }
    else
    {
        VariableSymbol* this_variable = ThisVariable();
        assert(this_variable);
        if (this_variable -> ACC_STATIC())
            s << " must be moved to a static initializer and enclosed in a "
              << "try statement which catches the exception, since static "
              << "initializers cannot throw checked exceptions.";
        else
        {
            assert(! ThisType() -> Anonymous());
            s << " must be moved to an instance initializer or constructor "
              << "and enclosed in a try statement which catches the "
              << "exception, or else every constructor in this class must be "
              << "declared to throw the exception.";
        }
    }
    return s.Array();
}


void Semantic::ProcessThrowStatement(Ast* stmt)
{
    AstThrowStatement* throw_statement = (AstThrowStatement*) stmt;

    ProcessExpression(throw_statement -> expression);
    TypeSymbol* type = throw_statement -> expression -> Type();

    if (type != control.no_type &&
        ! CanAssignmentConvertReference(control.Throwable(), type))
    {
        ReportSemError(SemanticError::EXPRESSION_NOT_THROWABLE,
                       throw_statement);
    }

    // Precise Rethrow Analysis
    bool precise_rethrow_handled = false;
    VariableSymbol* var = NULL;
    if (control.option.source >= JopaOption::SDK1_7 &&
        throw_statement -> expression -> symbol &&
        throw_statement -> expression -> symbol -> VariableCast())
    {
        var = throw_statement -> expression -> symbol -> VariableCast();
        if (var -> multithrown_exceptions)
            precise_rethrow_handled = true;
    }

    //
    // Since 'throw null' always generates NullPointerException, we do not
    // add it to the exception set; otherwise checked exception catch blocks
    // would be reachable because null is assignable to them.
    //
    SymbolSet* exception_set = TryExceptionTableStack().Top();
    if (exception_set && type != control.null_type)
    {
        if (precise_rethrow_handled)
        {
             for (unsigned i = 0; i < var -> multithrown_exceptions -> Length(); i++)
                  exception_set -> AddElement((*var -> multithrown_exceptions)[i]);
        }
        else
        {
            exception_set -> AddElement(type);
        }
    }

    if (precise_rethrow_handled)
    {
         for (unsigned i = 0; i < var -> multithrown_exceptions -> Length(); i++)
         {
              TypeSymbol* ex = (*var -> multithrown_exceptions)[i];
              if (UncaughtException(ex))
              {
                    ReportSemError(SemanticError::UNCAUGHT_THROWN_EXCEPTION,
                                   throw_statement, ex -> ContainingPackageName(),
                                   ex -> ExternalName(), UncaughtExceptionContext());
              }
         }
    }
    else if (UncaughtException(type))
        ReportSemError(SemanticError::UNCAUGHT_THROWN_EXCEPTION,
                       throw_statement, type -> ContainingPackageName(),
                       type -> ExternalName(), UncaughtExceptionContext());
}


void Semantic::ProcessTryStatement(Ast* stmt)
{
    AstTryStatement* try_statement = (AstTryStatement*) stmt;

    //
    // A try_statement containing a finally clause requires some extra local
    // variables in its immediately enclosing block. The first holds an
    // uncaught exception from the try or catch block.  The second holds the
    // return address of the jsr.  And if the method has a return type, 1-2
    // more slots are needed to hold the return value in the case of an
    // abrupt exit from a try or catch block.
    //
    // Meanwhile, statements within try or catch blocks cannot share local
    // variables with the finally block, because of a potential VerifyError if
    // the finally overwrites a register holding a monitor of an enclosed
    // synchronized statement during an abrupt exit.
    //
    AstBlock* enclosing_block = LocalBlockStack().TopBlock();
    int max_variable_index =
        enclosing_block -> block_symbol -> max_variable_index;

    // Java 7: Try-with-resources also needs helper variables for synthetic finally
    if (try_statement -> finally_clause_opt || try_statement -> NumResources() > 0)
    {
        BlockSymbol* enclosing_block_symbol = enclosing_block -> block_symbol;
        if (enclosing_block_symbol -> helper_variable_index < 0)
        {
            // first such statement encountered in enclosing block?
            enclosing_block_symbol -> helper_variable_index =
                enclosing_block_symbol -> max_variable_index;
            // Allocate slots:
            //   +0: primary exception (caught by catch-all handler)
            //   +1: JSR return address
            //   +2: close exception (for try-with-resources suppression in normal path)
            int base_slots = (try_statement -> NumResources() > 0) ? 3 : 2;
            enclosing_block_symbol -> max_variable_index += base_slots;
            if (ThisMethod() -> Type() != control.void_type)
            {
                if (control.IsDoubleWordType(ThisMethod() -> Type()))
                    enclosing_block_symbol -> max_variable_index += 2;
                else enclosing_block_symbol -> max_variable_index++;
            }
        }
    }

    // Process explicit finally block if present
    if (try_statement -> finally_clause_opt)
    {
        //
        // A finally block is processed in the environment of its immediate
        // enclosing block (as opposed to the environment of its associated
        // try block).
        //
        // Note that the finally block must be processed prior to the other
        // blocks in the try statement, because the computation of whether or
        // not an exception is catchable in a try statement depends on the
        // termination status of the associated finally block. See the
        // UncaughtException function. In addition, any variables used in
        // the finally block cannot be safely used in the other blocks.
        //
        AstBlock* block_body = try_statement -> finally_clause_opt -> block;
        block_body -> is_reachable = try_statement -> is_reachable;
        assert(! try_statement -> can_complete_normally);
        ProcessBlock(block_body);
        max_variable_index = block_body -> block_symbol -> max_variable_index;

        //
        // Warn about empty finally blocks.
        //
        if (block_body -> NumStatements() == 0)
        {
            ReportSemError(SemanticError::EJ_EMPTY_FINALLY_BLOCK, block_body);
        }

        //
        // If the finally ends abruptly, then it discards any throw generated
        // by the try or catch blocks.
        //
        if (! block_body -> can_complete_normally)
        {
            TryExceptionTableStack().Push(new SymbolSet());
            AbruptFinallyStack().Push(block_body -> nesting_level);
        }
    }

    //
    // Java 7: Process try-with-resources
    // Resources must be processed before catch clauses and must implement AutoCloseable
    //
    for (unsigned r = 0; r < try_statement -> NumResources(); r++)
    {
        AstLocalVariableStatement* resource = try_statement -> Resource(r);
        ProcessType(resource -> type);
        TypeSymbol* resource_type = resource -> type -> symbol;

        // Check that the resource type implements AutoCloseable (an interface)
        TypeSymbol* auto_closeable = control.AutoCloseable();
        if (resource_type != control.no_type && auto_closeable &&
            ! resource_type -> IsSubtype(auto_closeable))
        {
            ReportSemError(SemanticError::TYPE_NOT_AUTOCLOSEABLE,
                           resource,
                           resource_type -> ContainingPackageName(),
                           resource_type -> ExternalName());
        }

        // Process the resource variable declarations (usually just one)
        for (unsigned v = 0; v < resource -> NumVariableDeclarators(); v++)
        {
            AstVariableDeclarator* variable_declarator =
                resource -> VariableDeclarator(v);
            AstVariableDeclaratorId* name =
                variable_declarator -> variable_declarator_name;
            NameSymbol* name_symbol =
                lex_stream -> NameSymbol(name -> identifier_token);

            VariableSymbol* duplicate =
                LocalSymbolTable().FindVariableSymbol(name_symbol);
            if (duplicate)
            {
                ReportSemError(SemanticError::DUPLICATE_LOCAL_VARIABLE_DECLARATION,
                               name -> identifier_token, name_symbol -> Name(),
                               duplicate -> FileLoc());
            }

            AstBlock* block = LocalBlockStack().TopBlock();
            VariableSymbol* symbol =
                LocalSymbolTable().Top() -> InsertVariableSymbol(name_symbol);
            variable_declarator -> symbol = symbol;

            unsigned dims = resource_type -> num_dimensions + name -> NumBrackets();
            symbol -> SetType(resource_type -> GetArrayType(this, dims));
            symbol -> SetFlags(ProcessLocalModifiers(resource));
            symbol -> SetACC_FINAL();  // Resources are implicitly final
            symbol -> SetOwner(ThisMethod());
            symbol -> declarator = variable_declarator;
            symbol -> SetLocation();
            symbol -> SetLocalVariableIndex(block -> block_symbol ->
                                            max_variable_index++);
            if (control.IsDoubleWordType(symbol -> Type()))
                block -> block_symbol -> max_variable_index++;

            // Process the initializer
            if (variable_declarator -> variable_initializer_opt)
            {
                ProcessVariableInitializer(variable_declarator);
            }
        }
    }

    //
    // Note that the catch clauses are processed first - prior to processing
    // the main block - so that we can have their parameters available when we
    // are processing the main block, in case that block contains a throw
    // statement. See ProcessThrowStatement for more information. But since
    // a catch clause may be interrupted by an abrupt finally clause, we go
    // ahead and push the try block on the stack now, then use the field
    // processing_try_block later to mark the difference.
    //
    // Also, recall that the body of the catch blocks must not be
    // processed within the environment of the associated try whose
    // exceptions they are supposed to catch but within the immediate enclosing
    // block (which may itself be a try block).
    //
    TryStatementStack().Push(try_statement);
    for (unsigned i = 0; i < try_statement -> NumCatchClauses(); i++)
    {
        AstCatchClause* clause = try_statement -> CatchClause(i);
        AstFormalParameter* parameter = clause -> formal_parameter;
        assert(! parameter -> ellipsis_token_opt);
        AstVariableDeclaratorId* name =
            parameter -> formal_declarator -> variable_declarator_name;

        TypeSymbol* parm_type;

        // Java 7: Handle multi-catch
        if (clause -> NumUnionTypes() > 0)
        {
            // Multi-catch: process each union type and compute LUB
            parm_type = NULL;

            for (unsigned u = 0; u < clause -> NumUnionTypes(); u++)
            {
                AstType* union_type = clause -> UnionType(u);
                ProcessType(union_type);
                TypeSymbol* type = union_type -> symbol;

                if (type != control.no_type &&
                    ! type -> IsSubclass(control.Throwable()))
                {
                    ReportSemError(SemanticError::TYPE_NOT_THROWABLE,
                                   union_type,
                                   type -> ContainingPackageName(),
                                   type -> ExternalName());
                }

                // Compute LUB: find common superclass
                if (type != control.no_type)
                {
                    if (! parm_type)
                    {
                        parm_type = type;
                    }
                    else
                    {
                        // Find the most specific common superclass
                        // Walk up the inheritance chain of parm_type until we find
                        // a class that type is a subclass of
                        TypeSymbol* candidate = parm_type;
                        while (candidate && ! type -> IsSubclass(candidate))
                        {
                            candidate = candidate -> super;
                        }
                        if (candidate)
                            parm_type = candidate;
                        else
                            parm_type = control.Throwable();  // Fallback
                    }
                }

                // Check for duplicate types in the multi-catch
                for (unsigned prev = 0; prev < u; prev++)
                {
                    TypeSymbol* prev_type = clause -> UnionType(prev) -> symbol;
                    if (type == prev_type)
                    {
                        ReportSemError(SemanticError::DUPLICATE_EXCEPTION_IN_MULTICATCH,
                                       union_type,
                                       type -> ContainingPackageName(),
                                       type -> ExternalName());
                        break;
                    }
                    // Check for redundant types (one is subclass of another)
                    if (type -> IsSubclass(prev_type))
                    {
                        ReportSemError(SemanticError::REDUNDANT_EXCEPTION_IN_MULTICATCH,
                                       union_type,
                                       type -> ContainingPackageName(),
                                       type -> ExternalName(),
                                       prev_type -> ContainingPackageName(),
                                       prev_type -> ExternalName());
                        break;
                    }
                    if (prev_type -> IsSubclass(type))
                    {
                        ReportSemError(SemanticError::REDUNDANT_EXCEPTION_IN_MULTICATCH,
                                       clause -> UnionType(prev),
                                       prev_type -> ContainingPackageName(),
                                       prev_type -> ExternalName(),
                                       type -> ContainingPackageName(),
                                       type -> ExternalName());
                    }
                }
            }

            // Fallback if all types were errors
            if (! parm_type)
                parm_type = control.Throwable();
        }
        else
        {
            // Normal single-type catch
            ProcessType(parameter -> type);
            parm_type = parameter -> type -> symbol;
            if (name -> NumBrackets())
            {
                parm_type = parm_type ->
                    GetArrayType(this, (parm_type -> num_dimensions +
                                        name -> NumBrackets()));
            }
            if (! parm_type -> IsSubclass(control.Throwable()) &&
                parm_type != control.no_type)
            {
                ReportSemError(SemanticError::TYPE_NOT_THROWABLE, parameter,
                               parm_type -> ContainingPackageName(),
                               parm_type -> ExternalName());
                parm_type = control.no_type;
            }
        }

        NameSymbol* name_symbol =
            lex_stream -> NameSymbol(name -> identifier_token);
        VariableSymbol* duplicate =
            LocalSymbolTable().FindVariableSymbol(name_symbol);
        if (duplicate)
        {
            ReportSemError(SemanticError::DUPLICATE_LOCAL_VARIABLE_DECLARATION,
                           name -> identifier_token, name_symbol -> Name(),
                           duplicate -> FileLoc());
        }
        WarnOfAccessibleFieldWithName(SemanticError::LOCAL_SHADOWS_FIELD,
                                      name, name_symbol, false);

        AstBlock* block_body = clause -> block;
        
        //
        // Warn about empty catch blocks.
        //
        if (control.option.pedantic && block_body -> NumStatements() == 0)
            ReportSemError(SemanticError::EJ_EMPTY_CATCH_BLOCK, block_body);

        //
        // Guess that the number of elements in the table will not exceed the
        // number of statements + the clause parameter.
        //
        BlockSymbol* block = LocalSymbolTable().Top() ->
            InsertBlockSymbol(block_body -> NumStatements() + 1);
        block -> max_variable_index = max_variable_index;
        // LocalSymbolTable().Push(block -> Table()); // Deferred to Loop 2

        AccessFlags access_flags = ProcessFormalModifiers(parameter);

        // We must insert the variable into the block table, even if we don't push the table yet.
        // InsertVariableSymbol uses 'this' (the SymbolTable it is called on).
        // So we use block->Table()->InsertVariableSymbol
        VariableSymbol* symbol =
            block -> Table() -> InsertVariableSymbol(name_symbol);
        symbol -> SetFlags(access_flags);
        symbol -> SetType(parm_type);
        symbol -> SetOwner(ThisMethod());
        symbol -> SetLocalVariableIndex(block -> max_variable_index++);
        symbol -> MarkComplete();
        symbol -> declarator = parameter -> formal_declarator;
        symbol -> SetLocation();
        parameter -> formal_declarator -> symbol = symbol;
        clause -> parameter_symbol = symbol;

        //
        // Note that for the purpose of semantic checking we assume that
        // the body of the catch block is reachable. Whether or not the catch
        // statement can be executed at all is checked later.
        //
        block_body -> is_reachable = true;

        block_body -> block_symbol = block;
        // block_body -> nesting_level = LocalBlockStack().Size(); // Deferred
        // LocalBlockStack().Push(block_body); // Deferred

        // ProcessBlockStatements(block_body); // Deferred to Loop 2

        // LocalBlockStack().Pop(); // Deferred
        // LocalSymbolTable().Pop(); // Deferred

        /* Deferred to Loop 2
        //
        // Update the information for the block that immediately encloses
        // the current block.
        //
        if (LocalBlockStack().TopMaxEnclosedVariableIndex() <
            block -> max_variable_index)
        {
            LocalBlockStack().TopMaxEnclosedVariableIndex() =
                block -> max_variable_index;
        }

        //
        // If a catch clause block can complete normally, we assume
        // that the try statement can complete normally. This may
        // prove to be false later if we find out that the finally
        // clause cannot complete normally...
        //
        if (block_body -> can_complete_normally)
            try_statement -> can_complete_normally = true;

        block -> CompressSpace(); // space optimization
        */
    }

    //
    // Finally, process the main try block.
    //
    try_statement -> processing_try_block = true;
    SymbolSet* exception_set = new SymbolSet;
    TryExceptionTableStack().Push(exception_set);

    try_statement -> block -> is_reachable = try_statement -> is_reachable;
    AstBlock* block_body = try_statement -> block;
    //
    // Guess that the number of elements in the table will not exceed the
    // number of statements + 3. This padding allows extra variable
    // declarations in things like for-init, without expensive reallocation.
    //
    BlockSymbol* block = LocalSymbolTable().Top() ->
        InsertBlockSymbol(block_body -> NumStatements() + 3);
    block -> max_variable_index = max_variable_index;
    LocalSymbolTable().Push(block -> Table());

    block_body -> block_symbol = block;
    block_body -> nesting_level = LocalBlockStack().Size();
    LocalBlockStack().Push(block_body);

    ProcessBlockStatements(block_body);

    LocalBlockStack().Pop();
    LocalSymbolTable().Pop();

    //
    // Update the information for the block that immediately encloses the
    // current block.
    //
    if (LocalBlockStack().TopMaxEnclosedVariableIndex() <
        block -> max_variable_index)
    {
        LocalBlockStack().TopMaxEnclosedVariableIndex() =
            block -> max_variable_index;
    }

    block -> CompressSpace(); // space optimization

    if (try_statement -> block -> can_complete_normally)
        try_statement -> can_complete_normally = true;

    //
    // A catch block is reachable iff both of the following are true:
    //     . Some expression or throw statement in the try block is reachable
    //       and can throw an exception that is assignable to the parameter
    //       of the catch clause C. Note that every try block, including an
    //       empty one, is considered to throw unchecked exceptions.
    //     . There is no earlier catch block A in the try statement such that
    //       the type of C's parameter is the same as or a subclass of the
    //       type of A's parameter.
    //
    // Note that the use of the word assignable here is slightly misleading.
    // It does not mean assignable in the strict sense defined in section 5.2.
    // Using the strict definition of 5.2, the rule can be more accurately
    // stated as follows:
    //
    //    . Catchable Exception:
    //      Some expression or throw statement in the try block is reachable
    //      and can throw an exception S that is assignable to the parameter
    //      with type T (S is a subclass of T) of the catch clause C.
    //      In this case, when S is thrown it will definitely be caught by
    //      clause C.
    //
    //    . Convertible Exception:
    //      The type T of the parameter of the catch clause C is assignable to
    //      the type S (T is a subclass of S) of an exception that can be
    //      thrown by some expression or throw statement in the try block that
    //      is reachable. This rule captures the idea that at run time an
    //      object declared to be of type S can actually be an instance of an
    //      object of type T in which case it will be caught by clause C.
    //
    //
    // We pop the stack now so that the catch blocks are processed in the
    // environment of the enclosing block.
    //
    try_statement -> processing_try_block = false;
    TryStatementStack().Pop();
    TryExceptionTableStack().Pop();

    Tuple<TypeSymbol*> catchable_exceptions;
    Tuple<TypeSymbol*> convertible_exceptions;
    exception_set -> AddElement(control.Error());
    exception_set -> AddElement(control.RuntimeException());
    for (unsigned l = 0; l < try_statement -> NumCatchClauses(); l++)
    {
        AstCatchClause* clause = try_statement -> CatchClause(l);
        TypeSymbol* type = clause -> parameter_symbol -> Type();
        if (type == control.no_type)
            continue;
        unsigned initial_length = catchable_exceptions.Length() +
            convertible_exceptions.Length();
        unsigned initial_catchable = catchable_exceptions.Length();

        for (TypeSymbol* exception =
                 (TypeSymbol*) exception_set -> FirstElement();
             exception;
             exception = (TypeSymbol*) exception_set -> NextElement())
        {
            assert(exception != control.null_type);
            if (CanAssignmentConvertReference(type, exception))
                catchable_exceptions.Next() = exception;
            else if (CanAssignmentConvertReference(exception, type))
                convertible_exceptions.Next() = exception;
        }

        // Java 7: Populate precise rethrow exceptions
        if (control.option.source >= JopaOption::SDK1_7) {
            for (unsigned i = initial_catchable; i < catchable_exceptions.Length(); i++) {
                clause->parameter_symbol->AddMultiThrownException(catchable_exceptions[i]);
            }
        }

        //
        // No exception was found which can be caught by this clause.
        //
        if (catchable_exceptions.Length() + convertible_exceptions.Length() ==
            initial_length)
        {
            ReportSemError(SemanticError::UNREACHABLE_CATCH_CLAUSE,
                           clause -> formal_parameter,
                           type -> ContainingPackageName(),
                           type -> ExternalName());
        }
        else
        {
            //
            // Search to see if this clause duplicates a prior one.
            //
            AstCatchClause* previous_clause;
            unsigned k;
            for (k = 0; k < l; k++)
            {
                previous_clause = try_statement -> CatchClause(k);
                if (type -> IsSubclass(previous_clause -> parameter_symbol ->
                                       Type()))
                    break;
            }

            if (k < l)
            {
                FileLocation loc(lex_stream,
                                 (previous_clause -> formal_parameter ->
                                  RightToken()));
                TypeSymbol* prev_type =
                    previous_clause -> parameter_symbol -> Type();
                ReportSemError(SemanticError::BLOCKED_CATCH_CLAUSE,
                               clause -> formal_parameter,
                               type -> ContainingPackageName(),
                               type -> ExternalName(),
                               prev_type -> ContainingPackageName(),
                               prev_type -> ExternalName(),
                               loc.location);
            }
            else
            {
                clause -> block -> is_reachable = true;

                // Process Catch Body (Deferred from Loop 1)
                AstBlock* block_body = clause -> block;
                BlockSymbol* block = block_body -> block_symbol;

                LocalSymbolTable().Push(block -> Table());
                block_body -> nesting_level = LocalBlockStack().Size();
                LocalBlockStack().Push(block_body);

                ProcessBlockStatements(block_body);

                LocalBlockStack().Pop();
                LocalSymbolTable().Pop();

                // Update max_variable_index
                if (LocalBlockStack().TopMaxEnclosedVariableIndex() <
                    block -> max_variable_index)
                {
                    LocalBlockStack().TopMaxEnclosedVariableIndex() =
                        block -> max_variable_index;
                }

                if (block_body -> can_complete_normally)
                    try_statement -> can_complete_normally = true;

                block -> CompressSpace();
            }
        }
    }

    if (TryExceptionTableStack().Top())
    {
        //
        // First, remove all the thrown exceptions that are definitely caught
        // by the enclosing try statement. Then, add the remaining ones to the
        // set that must be caught by the immediately enclosing try statement.
        //
        for (unsigned i = 0; i < catchable_exceptions.Length(); i++)
            exception_set -> RemoveElement(catchable_exceptions[i]);
        TryExceptionTableStack().Top() -> Union(*exception_set);
    }
    delete exception_set;

    //
    // A try statement cannot complete normally if it contains a finally
    // clause that cannot complete normally. But remember that a continue
    // statement may have already marked the try statement as completing
    // normally. Clean up from above.
    //
    if (try_statement -> finally_clause_opt &&
        ! try_statement -> finally_clause_opt -> block -> can_complete_normally)
    {
        try_statement -> can_complete_normally = false;
        delete TryExceptionTableStack().Top();
        TryExceptionTableStack().Pop();
        AbruptFinallyStack().Pop();
    }
}


void Semantic::ProcessAssertStatement(Ast* stmt)
{
    AstAssertStatement* assert_statement = (AstAssertStatement*) stmt;

    ProcessExpression(assert_statement -> condition);

    TypeSymbol* type = assert_statement -> condition -> Type();
    // Allow Boolean wrapper type with auto-unboxing in Java 5+
    bool is_boolean = type == control.boolean_type ||
                      type == control.no_type ||
                      (control.option.source >= JopaOption::SDK1_5 &&
                       type == control.Boolean());
    if (! is_boolean)
    {
        ReportSemError(SemanticError::TYPE_NOT_BOOLEAN,
                       assert_statement -> condition,
                       type -> ContainingPackageName(),
                       type -> ExternalName());
    }
    //
    // If the condition is not constant true, store a reference to this class's
    // assert variable (creating it if necessary as a side-effect). However,
    // if we are not emitting asserts, we can skip this.
    //
    else if (type == control.boolean_type &&
             ! IsConstantTrue(assert_statement -> condition) &&
             ! control.option.noassert)
    {
        assert_statement -> assert_variable =
            ThisType() -> FindOrInsertAssertVariable();
    }

    if (assert_statement -> message_opt)
    {
        ProcessExpressionOrStringConstant(assert_statement -> message_opt);
        if (assert_statement -> message_opt -> Type() == control.void_type)
            ReportSemError(SemanticError::TYPE_IS_VOID,
                           assert_statement -> message_opt,
                           assert_statement -> message_opt -> Type() -> Name());
    }

    //
    // Asserts can complete normally iff reachable.
    //
    assert_statement -> can_complete_normally =
        assert_statement -> is_reachable;
}


void Semantic::ProcessEmptyStatement(Ast* stmt)
{
    AstEmptyStatement* empty_statement = (AstEmptyStatement*) stmt;

    //
    // An empty statement can complete normally iff it is reachable.
    //
    empty_statement -> can_complete_normally = empty_statement -> is_reachable;
}


TypeSymbol* Semantic::GetLocalType(AstDeclaredType* class_declaration)
{
    NameSymbol* name_symbol = lex_stream ->
        NameSymbol(class_declaration -> class_body -> identifier_token);
    TypeSymbol* type =
        LocalSymbolTable().Top() -> InsertTypeSymbol(name_symbol);

    TypeSymbol* this_type = ThisType();
    TypeSymbol* outermost_type = this_type -> outermost_type;
    if (! this_type -> local)
        this_type -> local = new SymbolSet;

    //
    // Anonymous and local classes can clash if we don't use both when
    // determining the id number of this class.
    //
    IntToWstring value(this_type -> NumLocalTypes() +
                       this_type -> NumAnonymousTypes() + 1);
    int length = this_type -> ExternalNameLength() + 1 + value.Length() +
        name_symbol -> NameLength(); // +1 for $
    wchar_t* external_name = new wchar_t[length + 1]; // +1 for '\0';
    wcscpy(external_name, this_type -> ExternalName());
    wcscat(external_name, (control.option.target < JopaOption::SDK1_5
                           ? StringConstant::US_DS : StringConstant::US_MI));
    wcscat(external_name, value.String());
    wcscat(external_name, name_symbol -> Name());

    type -> outermost_type = outermost_type;
    type -> SetExternalIdentity(control.FindOrInsertName(external_name,
                                                         length));
    this_type -> local -> AddElement(type);

    delete [] external_name;

    return type;
}


void Semantic::ProcessClassDeclaration(Ast* stmt)
{
    AstLocalClassStatement* class_statement = (AstLocalClassStatement*) stmt;
    AstDeclaredType* class_declaration = class_statement -> declaration;
    AstClassBody* class_body = class_declaration -> class_body;

    CheckNestedTypeDuplication(state_stack.Top(),
                               class_body -> identifier_token);

    TypeSymbol* inner_type = GetLocalType(class_declaration);
    inner_type -> outermost_type = ThisType() -> outermost_type;
    inner_type -> supertypes_closure = new SymbolSet;
    inner_type -> subtypes_closure = new SymbolSet;
    inner_type -> subtypes = new SymbolSet;
    inner_type -> semantic_environment =
        new SemanticEnvironment(this, inner_type, state_stack.Top());
    inner_type -> declaration = class_body;
    inner_type -> file_symbol = source_file_symbol;
    inner_type -> SetFlags(ProcessLocalClassModifiers(class_declaration));
    inner_type -> SetOwner(ThisMethod());
    //
    // Add 3 extra elements for padding. May need a default constructor and
    // other support elements.
    //
    inner_type -> SetSymbolTable(class_body -> NumClassBodyDeclarations() + 3);
    inner_type -> SetLocation();
    inner_type -> SetSignature(control);

    //
    // If a local class is not in a static region, it needs a place to store
    // the enclosing instance.
    //
    if (! StaticRegion())
        inner_type -> InsertThis0();

    // Save environment for processing bodies later.
    class_body -> semantic_environment = inner_type -> semantic_environment;
    CheckNestedMembers(inner_type, class_body);
    ProcessTypeHeaders(class_body);

    ProcessMembers(class_body);
    CompleteSymbolTable(class_body);
    ProcessExecutableBodies(class_body);
    UpdateLocalConstructors(inner_type);
}


void Semantic::ProcessThisCall(AstThisCall* this_call)
{
    TypeSymbol* this_type = ThisType();

    // Signal that we are about to process an explicit constructor invocation.
    ExplicitConstructorInvocation() = this_call;

    if (this_call -> type_arguments_opt &&
        control.option.source < JopaOption::SDK1_5)
    {
        ReportSemError(SemanticError::EXPLICIT_TYPE_ARGUMENTS_UNSUPPORTED,
                       this_call -> type_arguments_opt);
    }
    bool bad_argument = ProcessArguments(this_call -> arguments);
    if (! bad_argument)
    {
        MethodSymbol* constructor = FindConstructor(this_type, this_call,
                                                    this_call -> this_token,
                                                    this_call -> RightToken());
        if (constructor)
        {
            this_call -> symbol = constructor;
            MethodInvocationConversion(this_call -> arguments, constructor);
            for (unsigned i = 0; i < constructor -> NumThrows(); i++)
            {
                TypeSymbol* exception = constructor -> Throws(i);
                if (UncaughtException(exception))
                    ReportSemError(SemanticError::UNCAUGHT_EXPLICIT_THIS_EXCEPTION,
                                   this_call -> this_token,
                                   exception -> ContainingPackageName(),
                                   exception -> ExternalName());
            }

            //
            // Not all shadowed variables are known yet, but there is no
            // need to save context, since all shadow variables required by
            // the target constructor can just be passed along.
            //
        }
    }

    //
    // Signal that we are no longer processing an explicit constructor
    // invocation.
    //
    ExplicitConstructorInvocation() = NULL;
}


void Semantic::ProcessSuperCall(AstSuperCall* super_call)
{
    TypeSymbol* this_type = ThisType();
    if (super_call -> symbol)
    {
        // Symbol is pre-set for anonymous classes and enum constructors
        assert(this_type -> Anonymous() || this_type -> ACC_ENUM());
        return;
    }

    // Signal that we are about to process an explicit constructor invocation.
    ExplicitConstructorInvocation() = super_call;

    TypeSymbol* super_type = this_type -> super;
    if (! super_type)
    {
        assert(this_type == control.Object());
        ReportSemError(SemanticError::OBJECT_HAS_NO_SUPER_TYPE,
                       super_call -> super_token);
        return;
    }

    if (super_call -> base_opt)
    {
        ProcessExpression(super_call -> base_opt);

        TypeSymbol* expr_type = super_call -> base_opt -> Type();
        if (expr_type != control.no_type)
        {
            TypeSymbol* containing_type = super_type -> EnclosingType();
            if (expr_type -> Primitive() || expr_type == control.null_type)
            {
                ReportSemError(SemanticError::TYPE_NOT_REFERENCE,
                               super_call -> base_opt,
                               expr_type -> ExternalName());
                super_call -> base_opt -> symbol = control.no_type;
            }
            else if (! containing_type)
            {
                if (! super_type -> Bad())
                    ReportSemError(SemanticError::SUPER_TYPE_NOT_INNER_CLASS,
                                   super_call -> base_opt,
                                   super_type -> ContainingPackageName(),
                                   super_type -> ExternalName(),
                                   this_type -> ContainingPackageName(),
                                   this_type -> ExternalName(),
                                   expr_type -> ContainingPackageName(),
                                   expr_type -> ExternalName());
                super_call -> base_opt -> symbol = control.no_type;
            }
            //
            // JLS2 8.8.5.1: For an enclosing class O of the superclass,
            // the qualifying primary must be of type O or a subclass.
            //
            else if (! expr_type -> IsSubclass(containing_type))
            {
                ReportSemError(SemanticError::INVALID_ENCLOSING_INSTANCE,
                               super_call -> base_opt,
                               this_type -> ContainingPackageName(),
                               this_type -> ExternalName(),
                               containing_type -> ContainingPackageName(),
                               containing_type -> ExternalName(),
                               expr_type -> ContainingPackageName(),
                               expr_type -> ExternalName());
                super_call -> base_opt -> symbol = control.no_type;
            }
        }
    }
    else // (! super_call -> base_opt)
    {
        if (super_type && super_type -> EnclosingType())
            super_call -> base_opt =
                CreateAccessToType(super_call, super_type -> EnclosingType());
    }

    if (super_call -> type_arguments_opt &&
        control.option.source < JopaOption::SDK1_5)
    {
        ReportSemError(SemanticError::EXPLICIT_TYPE_ARGUMENTS_UNSUPPORTED,
                       super_call -> type_arguments_opt);
    }
    MethodSymbol* constructor = NULL;
    bool bad_argument = ProcessArguments(super_call -> arguments);
    if (! bad_argument)
    {
        constructor = FindConstructor(super_type, super_call,
                                      super_call -> super_token,
                                      super_call -> RightToken());
    }

    if (constructor)
    {
        if (constructor -> ACC_PRIVATE())
        {
            //
            // Create accessor constructor, and add extra null
            // to the constructor invocation.
            //
            constructor = super_type -> GetReadAccessConstructor(constructor);
            super_call -> arguments -> AddNullArgument();
        }

        super_call -> symbol = constructor;
        if (super_call -> base_opt)
        {
            assert(CanAssignmentConvertReference(super_type -> EnclosingType(),
                                                 super_call -> base_opt -> Type()));

            super_call -> base_opt =
                ConvertToType(super_call -> base_opt,
                              super_type -> EnclosingType());
        }
        MethodInvocationConversion(super_call -> arguments, constructor);

        //
        // Make sure that the throws signature of the constructor is
        // processed.
        //
        unsigned i;
        for (i = 0; i < constructor -> NumThrows(); i++)
        {
            TypeSymbol* exception = constructor -> Throws(i);
            if (UncaughtException(exception))
                ReportSemError(SemanticError::UNCAUGHT_EXPLICIT_SUPER_EXCEPTION,
                               super_call,
                               exception -> ContainingPackageName(),
                               exception -> ExternalName(),
                               constructor -> containing_type -> ContainingPackageName(),
                               constructor -> containing_type -> ExternalName());
        }

        //
        // A local super type may use enclosed local variables. If so, we
        // must add the parameters which allow the local type to
        // initialize its shadows.
        //
        if (super_type -> IsLocal())
        {
            unsigned param_count = super_type -> NumConstructorParameters();
            if (super_type -> LocalClassProcessingCompleted() && param_count)
            {
                super_call -> arguments -> AllocateLocalArguments(param_count);
                for (i = 0; i < param_count; i++)
                {
                    //
                    // We may need to create a shadow in the outermost
                    // local class enclosing the variable.
                    //
                    AstName* simple_name = compilation_unit ->
                        ast_pool -> GenName(super_call -> super_token);
                    VariableSymbol* accessor =
                        FindLocalVariable(super_type -> ConstructorParameter(i),
                                          this_type);
                    simple_name -> symbol = accessor;
                    TypeSymbol* owner = accessor -> ContainingType();
                    if (owner != this_type)
                        CreateAccessToScopedVariable(simple_name, owner);
                    super_call -> arguments -> AddLocalArgument(simple_name);
                }
            }
            else
            {
                //
                // We are within body of super_type; save processing for
                // later, since not all shadows may be known yet. See
                // ProcessClassDeclaration.
                //
                super_type -> AddLocalConstructorCallEnvironment
                    (GetEnvironment(super_call -> arguments));
            }
        }
    }

    //
    // Signal that we are no longer processing an explicit constructor
    // invocation.
    //
    ExplicitConstructorInvocation() = NULL;
}


//
// Checks that types in a throws clause extend Throwable. throws_list is NULL
// except in pedantic mode, where it is used to detect duplicates.
//
void Semantic::CheckThrow(AstTypeName* throw_expression,
                          Tuple<AstTypeName*>* throws_list)
{
    TypeSymbol* throw_type = throw_expression -> symbol;
    if (throw_type -> Bad())
        return;

    if (throw_type -> ACC_INTERFACE())
    {
        ReportSemError(SemanticError::NOT_A_CLASS, throw_expression,
                       throw_type -> ContainingPackageName(),
                       throw_type -> ExternalName());
    }
    else if (! throw_type -> IsSubclass(control.Throwable()))
    {
        ReportSemError(SemanticError::TYPE_NOT_THROWABLE, throw_expression,
                       throw_type -> ContainingPackageName(),
                       throw_type -> ExternalName());
    }
    else if (throw_type == control.Exception() ||
             throw_type == control.Throwable())
    {
        ReportSemError(SemanticError::EJ_OVERLY_GENERAL_THROWS_CLAUSE,
                       throw_expression);
    }
    else if (control.option.pedantic)
    {
        assert(throws_list);
        if (! CheckedException(throw_type))
            ReportSemError(SemanticError::UNCHECKED_THROWS_CLAUSE_CLASS,
                           throw_expression,
                           throw_type -> ContainingPackageName(),
                           throw_type -> ExternalName());
        else
        {
            bool add = true;
            for (unsigned i = 0; i < throws_list -> Length(); i++)
            {
                AstTypeName* other_expr = (*throws_list)[i];
                TypeSymbol* other_type = other_expr -> symbol;
                if (other_type == throw_type)
                {
                    ReportSemError(SemanticError::DUPLICATE_THROWS_CLAUSE_CLASS,
                                   throw_expression,
                                   throw_type -> ContainingPackageName(),
                                   throw_type -> ExternalName());
                    add = false;
                }
                else if (throw_type -> IsSubclass(other_type))
                {
                    ReportSemError(SemanticError::REDUNDANT_THROWS_CLAUSE_CLASS,
                                   throw_expression,
                                   throw_type -> ContainingPackageName(),
                                   throw_type -> ExternalName(),
                                   other_type -> ContainingPackageName(),
                                   other_type -> ExternalName());
                    add = false;
                }
                else if (other_type -> IsSubclass(throw_type))
                {
                    ReportSemError(SemanticError::REDUNDANT_THROWS_CLAUSE_CLASS,
                                   other_expr,
                                   other_type -> ContainingPackageName(),
                                   other_type -> ExternalName(),
                                   throw_type -> ContainingPackageName(),
                                   throw_type -> ExternalName());
                    //
                    // Remove other type from the list, to reduce extra errors.
                    //
                    int last_index = throws_list -> Length() - 1;
                    (*throws_list)[i] = (*throws_list)[last_index];
                    throws_list -> Reset(last_index);
                    i--;
                }
            }
            if (add)
                throws_list -> Next() = throw_expression;
        }
    }
}


void Semantic::ProcessMethodBody(AstMethodDeclaration* method_declaration)
{
    MethodSymbol* this_method = ThisMethod();

    if (method_declaration -> NumThrows())
    {
        Tuple<AstTypeName*>* throws_list = NULL;
        if (control.option.pedantic)
            throws_list = new Tuple<AstTypeName*>
                (method_declaration -> NumThrows());
        for (unsigned k = 0; k < method_declaration -> NumThrows(); k++)
            CheckThrow(method_declaration -> Throw(k), throws_list);
        delete throws_list;
    }

    if (method_declaration -> method_body_opt)
    {
        AstMethodBody* method_body = method_declaration -> method_body_opt;
        if (method_body -> explicit_constructor_opt)
            ReportSemError(SemanticError::MISPLACED_EXPLICIT_CONSTRUCTOR,
                           method_body -> explicit_constructor_opt);
        method_body -> block_symbol = this_method -> block_symbol;
        method_body -> nesting_level = LocalBlockStack().Size();
        LocalBlockStack().Push(method_body);

        ProcessBlockStatements(method_body);

        LocalBlockStack().Pop();

        if (method_body -> can_complete_normally)
        {
            if (this_method -> Type() == control.void_type)
            {
                AstReturnStatement* return_statement =
                    compilation_unit -> ast_pool -> GenReturnStatement();
                return_statement -> return_token =
                    method_body -> right_brace_token;
                return_statement -> semicolon_token =
                    method_body -> right_brace_token;
                return_statement -> is_reachable = true;
                method_body -> can_complete_normally = false;
                method_body -> AddStatement(return_statement);
            }
            else
            {
                ReportSemError(SemanticError::TYPED_METHOD_WITH_NO_RETURN,
                               method_declaration -> type -> LeftToken(),
                               method_declaration -> method_declarator -> identifier_token,
                               this_method -> Header(),
                               this_method -> Type() -> Name());
            }
        }

        if (this_method -> ACC_ABSTRACT() || this_method -> ACC_NATIVE())
        {
            ReportSemError(SemanticError::ABSTRACT_METHOD_WITH_BODY,
                           method_declaration, this_method -> Header());
        }
    }
    else if (! (this_method -> ACC_ABSTRACT() || this_method -> ACC_NATIVE()))
    {
        ReportSemError(SemanticError::NON_ABSTRACT_METHOD_WITHOUT_BODY,
                       method_declaration, this_method -> Header());
    }

    this_method -> block_symbol -> CompressSpace(); // space optimization
}


void Semantic::ProcessConstructorBody(AstConstructorDeclaration* constructor_declaration)
{
    TypeSymbol* this_type = ThisType();
    MethodSymbol* this_method = ThisMethod();

    if (constructor_declaration -> NumThrows())
    {
        Tuple<AstTypeName*>* throws_list = NULL;
        if (control.option.pedantic)
            throws_list = new Tuple<AstTypeName*>
                (constructor_declaration -> NumThrows());
        for (unsigned k = 0; k < constructor_declaration -> NumThrows(); k++)
            CheckThrow(constructor_declaration -> Throw(k), throws_list);
        delete throws_list;
    }

    AstMethodBody* constructor_block =
        constructor_declaration -> constructor_body;

    AstSuperCall* super_call = NULL;
    TypeSymbol* super_type = this_type -> super;

    if (constructor_block -> explicit_constructor_opt)
    {
        super_call =
            constructor_block -> explicit_constructor_opt -> SuperCallCast();
        if (super_call)
            ProcessSuperCall(super_call);
        else
        {
            AstThisCall* this_call = constructor_block ->
                explicit_constructor_opt -> ThisCallCast();
            assert(this_call);
            ProcessThisCall(this_call);
        }
    }
    else if (super_type)
    {
        TokenIndex loc = constructor_block -> LeftToken();
        super_call = compilation_unit -> ast_pool -> GenSuperCall();
        super_call -> super_token = loc;
        super_call -> arguments =
            compilation_unit -> ast_pool -> GenArguments(loc, loc);
        super_call -> semicolon_token = loc;

        // For enum constructors (except java.lang.Enum itself), pass name and ordinal
        // to the super(String, int) constructor of java.lang.Enum
        if (this_type -> IsEnum() && this_type != control.Enum())
        {
            super_call -> arguments -> AllocateArguments(2);

            // Get the name and ordinal parameter symbols from the constructor's block symbol
            VariableSymbol* name_param = this_method -> block_symbol -> FindVariableSymbol(
                control.FindOrInsertName(L"$name", 5));
            VariableSymbol* ordinal_param = this_method -> block_symbol -> FindVariableSymbol(
                control.FindOrInsertName(L"$ordinal", 8));

            // Create name argument reference
            AstName* name_ref = compilation_unit -> ast_pool -> GenName(loc);
            if (name_param)
                name_ref -> symbol = name_param;
            super_call -> arguments -> AddArgument(name_ref);

            // Create ordinal argument reference
            AstName* ordinal_ref = compilation_unit -> ast_pool -> GenName(loc);
            if (ordinal_param)
                ordinal_ref -> symbol = ordinal_param;
            super_call -> arguments -> AddArgument(ordinal_ref);

            // Set the symbol to the Enum(String, int) constructor
            super_call -> symbol = control.Enum_InitMethod();
        }

        constructor_block -> explicit_constructor_opt = super_call;

        ProcessSuperCall(super_call);
    }

    //
    // Guess that the number of elements will not exceed the number of
    // statements.
    //
    int table_size = constructor_block -> NumStatements();
    BlockSymbol* block = LocalSymbolTable().Top() ->
        InsertBlockSymbol(table_size);
    block -> max_variable_index =
        this_method -> block_symbol -> max_variable_index;
    LocalSymbolTable().Push(block -> Table());

    constructor_block -> block_symbol = block;
    constructor_block -> nesting_level = LocalBlockStack().Size();
    LocalBlockStack().Push(constructor_block);

    ProcessBlockStatements(constructor_block);

    if (constructor_block -> can_complete_normally)
    {
        AstReturnStatement* return_statement =
            compilation_unit -> ast_pool -> GenReturnStatement();
        return_statement -> return_token =
            constructor_block -> right_brace_token;
        return_statement -> semicolon_token =
            constructor_block -> right_brace_token;
        return_statement -> is_reachable = true;
        constructor_block -> can_complete_normally = false;
        constructor_block -> AddStatement(return_statement);
    }

    LocalBlockStack().Pop();
    LocalSymbolTable().Pop();

    //
    // Update the local variable info for the main block associated with this
    // constructor.
    //
    if (this_method -> block_symbol -> max_variable_index <
        block -> max_variable_index)
    {
        this_method -> block_symbol -> max_variable_index =
            block -> max_variable_index;
    }

    block -> CompressSpace(); // space optimization
}


void Semantic::ProcessExecutableBodies(AstClassBody* class_body)
{
    if (compilation_unit -> BadCompilationUnitCast())
        return; // errors were detected, exit now

    state_stack.Push(class_body -> semantic_environment);
    TypeSymbol* this_type = ThisType();
    unsigned i;

    assert(this_type -> HeaderProcessed());
    assert(this_type -> ConstructorMembersProcessed());
    assert(this_type -> MethodMembersProcessed());
    assert(this_type -> FieldMembersProcessed());

    // All variable declarations have already been processed.
    if (! this_type -> ACC_INTERFACE())
    {
        //
        // Compute the set of instance final variables which must be assigned
        // in every constructor.
        //
        Tuple<VariableSymbol*> unassigned(FinalFields() -> Length());
        for (i = 0; i < FinalFields() -> Length(); i++)
        {
            VariableSymbol* variable_symbol = (*FinalFields())[i];
            if (! DefinitelyAssignedVariables() -> da_set[i])
            {
                // Enum constants are static final, initialized in static initializer
                if (variable_symbol -> ACC_ENUM())
                    continue;
                assert(! variable_symbol -> ACC_STATIC());
                unassigned.Next() = variable_symbol;
            }
        }
        if (class_body -> NumConstructors() == 0)
        {
            //
            // Issue an error for each unassigned final.
            //
            for (i = 0; i < unassigned.Length(); i++)
            {
                ReportSemError(SemanticError::UNINITIALIZED_FINAL_VARIABLE,
                               unassigned[i] -> declarator,
                               unassigned[i] -> Name());
            }
            //
            // Process the body of the default constructor, if there is one (if
            // the class is invalid, one might not exist).
            //
            if (class_body -> default_constructor)
            {
                ThisMethod() =
                    class_body -> default_constructor -> constructor_symbol;
                LocalSymbolTable().Push(ThisMethod() -> block_symbol ->
                                        Table());
                ProcessConstructorBody(class_body -> default_constructor);
                LocalSymbolTable().Pop();
                ThisMethod() -> max_block_depth = 1;
            }
        }
        else
        {
            DefinitePair initial_state(*DefinitelyAssignedVariables());
            for (i = 0; i < class_body -> NumConstructors(); i++)
            {
                AstConstructorDeclaration* constructor_decl =
                    class_body -> Constructor(i);
                MethodSymbol* this_method =
                    constructor_decl -> constructor_symbol;
                if (! this_method)
                    continue;
                ThisMethod() = this_method;
                AstMethodBody* constructor_block =
                    constructor_decl -> constructor_body;

                LocalSymbolTable().Push(this_method -> block_symbol ->
                                        Table());
                LocalBlockStack().max_size = 0;
                ProcessConstructorBody(constructor_decl);
                LocalSymbolTable().Pop();
                this_method -> max_block_depth = LocalBlockStack().max_size;
                //
                // Each constructor starts from the same initial definite
                // assignment status, except those which call this() start with
                // all fields definitely assigned.
                //
                if (constructor_block -> explicit_constructor_opt &&
                    (constructor_block -> explicit_constructor_opt ->
                     ThisCallCast()))
                {
                    DefinitelyAssignedVariables() -> AssignAll();
                    DefiniteConstructorBody(constructor_decl);
                }
                else
                {
                    *DefinitelyAssignedVariables() = initial_state;
                    DefiniteConstructorBody(constructor_decl);
                    for (unsigned k = 0; k < unassigned.Length(); k++)
                    {
                        VariableSymbol* variable_symbol = unassigned[k];
                        if (! DefinitelyAssignedVariables() ->
                            da_set[variable_symbol -> LocalVariableIndex()])
                        {
                            ReportSemError(SemanticError::UNINITIALIZED_FINAL_VARIABLE_IN_CONSTRUCTOR,
                                           constructor_decl,
                                           variable_symbol -> Name());
                        }
                    }
                }
            }
            ConstructorCycleChecker cycle_checker(class_body);
        }
    }

    //
    // No need to worry about private access constructors, as we have already
    // done all necessary processing when creating them. Following all
    // constructors, all fields are definitely assigned, and are no longer
    // treated as blank finals.
    //
    DefinitelyAssignedVariables() -> AssignAll();
    BlankFinals() -> SetEmpty();

    for (i = 0; i < class_body -> NumMethods(); i++)
    {
        AstMethodDeclaration* method_decl = class_body -> Method(i);
        ThisMethod() = method_decl -> method_symbol;
        MethodSymbol* this_method = ThisMethod();
        if (this_method)
        {
            LocalSymbolTable().Push(this_method -> block_symbol -> Table());
            LocalBlockStack().max_size = 0;
            unsigned start_num_errors = NumErrors();
            ProcessMethodBody(method_decl);
            LocalSymbolTable().Pop();
            this_method -> max_block_depth = LocalBlockStack().max_size;
            if (NumErrors() == start_num_errors)
                DefiniteMethodBody(method_decl);
        }
    }
    ThisMethod() = NULL;

    //
    // Recursively process all inner types.
    //
    for (i = 0; i < class_body -> NumNestedClasses(); i++)
    {
        AstClassDeclaration* declaration = class_body -> NestedClass(i);
        if (declaration -> class_body -> semantic_environment)
            ProcessExecutableBodies(declaration -> class_body);
    }
    for (i = 0; i < class_body -> NumNestedInterfaces(); i++)
    {
        AstInterfaceDeclaration* declaration =
            class_body -> NestedInterface(i);
        if (declaration -> class_body -> semantic_environment)
            ProcessExecutableBodies(declaration -> class_body);
    }

    DefiniteCleanUp();
    state_stack.Pop();
}



} // Close namespace Jopa block
