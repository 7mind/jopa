#include "parser.h"
#include "ast.h"
#include "stream.h"


namespace Jopa { // Open namespace Jopa block
void Parser::ReallocateStacks()
{
    int old_stack_length = stack_length;

    stack_length += STACK_INCREMENT;

    assert(stack_length <= SHRT_MAX);

    int* old_stack = stack;
    stack = new int[stack_length];
    if (old_stack_length > 0)
        memcpy(stack, old_stack, old_stack_length * sizeof(int));
    delete [] old_stack;

    Location* old_location_stack = location_stack;
    location_stack = new Location[stack_length];
    if (old_stack_length > 0)
        memcpy(location_stack, old_location_stack, old_stack_length * sizeof(Location));
    delete [] old_location_stack;

    Ast** old_parse_stack = parse_stack;
    parse_stack = new Ast*[stack_length];
    if (old_stack_length > 0)
        memcpy(parse_stack, old_parse_stack, old_stack_length * sizeof(Ast*));
    delete [] old_parse_stack;
    // The first time through, we initialize parse_stack[0] to NULL.
    parse_stack[old_stack_length] = NULL;

    int* old_temp_stack = temp_stack;
    temp_stack = new int[stack_length];
    if (old_stack_length > 0)
        memcpy(temp_stack, old_temp_stack, old_stack_length * sizeof(int));
    delete [] old_temp_stack;
}


AstListNode *Parser::AllocateListNode()
{
    AstListNode *p;

    if (free_list_nodes)
    {
        p = free_list_nodes;
        free_list_nodes = free_list_nodes -> next;
    }
    else p = list_node_pool -> NewListNode();

    return p;
}


void Parser::FreeCircularList(AstListNode* tail)
{
    if (tail)
    {
        AstListNode* root = tail -> next;
        tail -> next = free_list_nodes;
        free_list_nodes = root;
    }
}


AstPackageDeclaration* Parser::PackageHeaderParse(LexStream* lex_stream_,
                                                  StoragePool* ast_pool_)
{
    AstPackageDeclaration* package_declaration = NULL;
    lex_stream_ -> Reset();

    if (lex_stream_ -> PackageToken())
    {
        ast_pool = ast_pool_;
        list_node_pool = new StoragePool(lex_stream_ -> NumTokens());
        free_list_nodes = NULL;

        parse_package_header_only = true;
        // We are parsing the whole input and not just a segment.
        end_token = LexStream::LEX_INFINITY;
        lex_stream = lex_stream_;
        Ast *ast = HeaderParse();
        parse_package_header_only = false;

        if (ast)
        {
            AstCompilationUnit *compilation_unit =
                ast -> CompilationUnitCast();
            if (compilation_unit &&
                ! compilation_unit -> BadCompilationUnitCast())
            {
                package_declaration =
                    compilation_unit -> package_declaration_opt;
            }
        }

        delete list_node_pool;
    }

    return package_declaration;
}


AstCompilationUnit* Parser::HeaderParse(LexStream* lex_stream_,
                                        StoragePool* ast_pool_)
{
    lex_stream_ -> Reset();

    body_pool = new StoragePool(lex_stream_ -> NumTokens());
    ast_pool = (ast_pool_ ? ast_pool_ : body_pool);
    list_node_pool = new StoragePool(lex_stream_ -> NumTokens());
    free_list_nodes = NULL;
    AstCompilationUnit *compilation_unit = NULL;

    parse_header_only = true;
    // We are parsing the whole input and not just a segment.
    end_token = LexStream::LEX_INFINITY;
    lex_stream = lex_stream_;
    Ast *ast = HeaderParse();
    parse_header_only = false;

    if (ast)
    {
        compilation_unit = ast -> CompilationUnitCast();
        if (compilation_unit &&
            ! compilation_unit -> BadCompilationUnitCast())
        {
            if (compilation_unit -> NumTypeDeclarations() == 0)
                compilation_unit -> MarkEmpty();
        }
    }

    //
    // If we succesfully parsed a compilation unit, allocate a storage pool
    // for it. Subtract the amount of space that's already been allocated for
    // the headers from the estimate for the bodies.
    //
    if (compilation_unit)
         compilation_unit -> ast_pool = body_pool;
    else delete body_pool;

    delete list_node_pool; // free the pool of list nodes

    return compilation_unit;
}


Ast* Parser::HeaderParse()
{
    TokenObject curtok = lex_stream -> Gettoken();
    int act = START_STATE,
              current_kind = lex_stream -> Kind(curtok);

/*****************************************************************/
/* Start parsing.                                                */
/*****************************************************************/
    state_stack_top = -1;

    //
    // Process a terminal
    //
    while (true)
    {
        if (++state_stack_top >= stack_length)
            ReallocateStacks();

        stack[state_stack_top] = act;
        location_stack[state_stack_top] = Loc(curtok);

        act = t_action(act, current_kind, lex_stream);

        if (act <= NUM_RULES)
            state_stack_top--; // make reduction look like a shift-reduce
        else if (act > ERROR_ACTION)
        {
            curtok = lex_stream -> Gettoken();
            current_kind = lex_stream -> Kind(curtok);

            act -= ERROR_ACTION;
        }
        else if (act < ACCEPT_ACTION)
        {
            curtok = lex_stream -> Gettoken();
            current_kind = lex_stream -> Kind(curtok);

            continue;
        }
        else break;

        //
        // Process a non_terminal
        //
        do
        {
            state_stack_top -= (rhs[act] - 1);
            (this ->* rule_action[act])();
            act = nt_action(stack[state_stack_top], lhs[act]);
        } while (act <= NUM_RULES);
    } /* process_terminal */

    if (act == ERROR_ACTION)
    {
        //
        // If any error is found in a package declaration, do not try to
        // repair it.
        //
        if (! parse_package_header_only)
            RepairParse(curtok);

        if (parse_stack[0] && parse_stack[0] -> CompilationUnitCast())
            ((AstCompilationUnit*) parse_stack[0]) -> MarkBad();
        else parse_stack[0] = NULL;
    }

    return parse_stack[0];
}


bool Parser::BodyParse(LexStream* lex_stream_, AstClassBody* class_body)
{
    assert(class_body -> UnparsedClassBodyCast());

    lex_stream = lex_stream_;
    ast_pool = class_body -> pool;
    body_pool = class_body -> pool;
    list_node_pool = new StoragePool(lex_stream_ -> NumTokens());
    free_list_nodes = NULL;

    bool success = Body(class_body);

    delete list_node_pool; // free the pool of list nodes

    class_body -> MarkParsed();

    return success;
}


bool Parser::Body(AstClassBody* class_body)
{
    bool errors_detected = false;
    unsigned i;

    for (i = 0; i < class_body -> NumConstructors(); i++)
    {
        AstConstructorDeclaration* constructor_decl =
            class_body -> Constructor(i);

        if (constructor_decl -> constructor_symbol)
        {
            AstMethodBody* block = constructor_decl -> constructor_body;
            end_token = block -> right_brace_token; // last token in the body

            AstMethodBody* new_body = ParseSegment(block -> left_brace_token);

            if (! new_body)
                errors_detected = true;
            else constructor_decl -> constructor_body = new_body;
        }
    }

    for (i = 0; i < class_body -> NumMethods(); i++)
    {
        AstMethodDeclaration* method_decl = class_body -> Method(i);
        if (method_decl -> method_symbol && method_decl -> method_body_opt)
        {
            AstMethodBody* block = method_decl -> method_body_opt;
            end_token = block -> right_brace_token;
            AstMethodBody* new_block = ParseSegment(block -> left_brace_token);
            if (! new_block) // a bad block ?
                errors_detected = true;
            else method_decl -> method_body_opt = new_block;
        }
    }

    for (i = 0; i < class_body -> NumNestedClasses(); i++)
        errors_detected = errors_detected ||
            ! Body(class_body -> NestedClass(i) -> class_body);
    for (i = 0; i < class_body -> NumNestedInterfaces(); i++)
        errors_detected = errors_detected ||
            ! Body(class_body -> NestedInterface(i) -> class_body);
    return ! errors_detected;
}


bool Parser::InitializerParse(LexStream* stream, AstClassBody* class_body)
{
    lex_stream = stream;
    ast_pool = class_body -> pool;
    body_pool = class_body -> pool;
    list_node_pool = new StoragePool(stream -> NumTokens());
    free_list_nodes = NULL;

    bool success = Initializer(class_body);

    delete list_node_pool; // free the pool of list nodes
    return success;
}


bool Parser::Initializer(AstClassBody* class_body)
{
    bool errors_detected = false;
    unsigned i;

    for (i = 0; i < class_body -> NumStaticInitializers(); i++)
    {
         AstMethodBody* block = class_body -> StaticInitializer(i) -> block;
         end_token = block -> right_brace_token; // last token in the body
         class_body -> StaticInitializer(i) -> block =
             ParseSegment(block -> left_brace_token);
        if (! class_body -> StaticInitializer(i) -> block)
        {
            errors_detected = true;
            // Restore old empty block.
            class_body -> StaticInitializer(i) -> block = block;
        }
    }
    for (i = 0; i < class_body -> NumInstanceInitializers(); i++)
    {
        AstMethodBody* block = class_body -> InstanceInitializer(i) -> block;
        end_token = block -> right_brace_token; // last token in the body
        class_body -> InstanceInitializer(i) -> block =
            ParseSegment(block -> left_brace_token);
        if (! class_body -> InstanceInitializer(i) -> block)
        {
            errors_detected = true;
            // Restore old empty block.
            class_body -> InstanceInitializer(i) -> block = block;
        }
    }
    for (i = 0; i < class_body -> NumNestedClasses(); i++)
        errors_detected = errors_detected ||
            ! Initializer(class_body -> NestedClass(i) -> class_body);
    for (i = 0; i < class_body -> NumNestedInterfaces(); i++)
        errors_detected = errors_detected ||
            ! Initializer(class_body -> NestedInterface(i) -> class_body);
    return ! errors_detected;
}


AstMethodBody* Parser::ParseSegment(TokenObject start_token)
{
    //
    // The next call to Gettoken will return the start_token.
    // However, we initialize curtok to start_token in order
    // to obtain a valid location for the BodyMarker.
    //
    lex_stream -> Reset(start_token);
    TokenObject curtok = start_token; // get the location of the start_token
    int act = START_STATE,
              current_kind = TK_BodyMarker;

/*****************************************************************/
/* Start parsing.                                                */
/*****************************************************************/
    state_stack_top = -1;

    //
    // Process a terminal
    //
    while (true)
    {
        if (++state_stack_top >= stack_length)
            ReallocateStacks();

        stack[state_stack_top] = act;
        location_stack[state_stack_top] = Loc(curtok);

        act = t_action(act, current_kind, lex_stream);

        if (act <= NUM_RULES)
            state_stack_top--; // make reduction look like a shift-reduce
        else if (act > ERROR_ACTION)
        {
            curtok = lex_stream -> Gettoken(end_token);
            current_kind = lex_stream -> Kind(curtok);

            act -= ERROR_ACTION;
        }
        else if (act < ACCEPT_ACTION)
        {
            curtok = lex_stream -> Gettoken(end_token);
            current_kind = lex_stream -> Kind(curtok);

            continue;
        }
        else
        {
            // ERROR_ACTION or ACCEPT_ACTION
            // Before giving up on errors, try to handle multi-catch
            if (act == ERROR_ACTION && current_kind == TK_OR &&
                TryHandleMultiCatch(curtok, current_kind))
            {
                // Multi-catch handled, we need to continue with new state
                // Set act to current state and continue
                act = stack[state_stack_top];
                continue;
            }
            break;
        }

        //
        // Process a nonterminal
        //
        do
        {
            state_stack_top -= (rhs[act] - 1);
            (this ->* rule_action[act])();
            act = nt_action(stack[state_stack_top], lhs[act]);
        } while (act <= NUM_RULES);
    } /* process_terminal */

    if (act == ERROR_ACTION)
    {
        RepairParse(curtok);

        parse_stack[0] = NULL;
    }

    return DYNAMIC_CAST<AstMethodBody *> (parse_stack[0]);
}


void Parser::RepairParse(TokenObject curtok)
{
    //
    // Repair an error
    //
    while (true)
    {
        //
        // Pop state stack up to first state that had an
        // action on the error token.  The net effect is to
        // remove all default reductions on an empty rule
        // caused by the error token.
        //
        int k;
        for (k = state_stack_top - 1;
             k >= 0 && location_stack[k] == Loc(curtok); k--);
        k++;

        state_stack_top = k;

        ErrorRepair(curtok);

        curtok = lex_stream -> Gettoken(end_token);
        int act = stack[state_stack_top--];
        int current_kind = lex_stream -> Kind(curtok);

        //
        // Process a terminal
        //
        while (true)
        {
            if (++state_stack_top >= stack_length)
                 ReallocateStacks();

            stack[state_stack_top] = act;
            location_stack[state_stack_top] = Loc(curtok);

            act = t_action(act, current_kind, lex_stream);

            if (act <= NUM_RULES)
                state_stack_top--; // make reduction look like a shift-reduce
            else if (act > ERROR_ACTION)
            {
                curtok = lex_stream -> Gettoken(end_token);
                current_kind = lex_stream -> Kind(curtok);

                act -= ERROR_ACTION;
            }
            else if (act < ACCEPT_ACTION)
            {
                curtok = lex_stream -> Gettoken(end_token);
                current_kind = lex_stream -> Kind(curtok);

                continue;
            }
            //
            // Since the null string is a valid Java program, this function
            // will always succeed even if it has to delete the whole input.
            //
            else if (act == ACCEPT_ACTION)
                 return;
            else
                // loop around and keep trying until something is accepted.
                break;

            //
            // Process a nonterminal
            //
            do
            {
                state_stack_top -= (rhs[act] - 1);
                (this ->* rule_action[act])();
                act = nt_action(stack[state_stack_top], lhs[act]);
            } while (act <= NUM_RULES);
        } /* process_terminal */
    }
}


//
// This routine is invoked when an error is encountered in a "repair"
// parser. It will place the parser back into a working configuration
// by adjusting the state stack, the current token and the buffer.
//
void Parser::ErrorRepair(TokenObject error_token)
{
    SecondaryRepairInfo repair;

    repair.code = ERROR_CODE;
    do
    {
        repair.distance = 0;
        repair.num_deletions = state_stack_top + BUFF_UBOUND;

        buffer[1] = error_token;
        buffer[0] = lex_stream -> Previous(buffer[1]);

        for (int k = 2; k < BUFF_SIZE; k++)
            buffer[k] = lex_stream -> Next(buffer[k - 1]);

        int last_index;
        for (last_index = MAX_DISTANCE - 1;
             last_index >= 1 &&
                 lex_stream -> Kind(buffer[last_index]) == EOFT_SYMBOL;
             last_index--);
        last_index++;

        repair = ErrorSurgery(stack, state_stack_top, last_index, repair);
        error_token = buffer[MAX_DISTANCE - MIN_DISTANCE + 2];
    } while (repair.code == 0);

    state_stack_top = repair.stack_position;
    lex_stream -> Reset(buffer[repair.buffer_position]);
}


//
// Keep cutting "stuff" out around the error until a stable configuration
// is found.
//
SecondaryRepairInfo Parser::ErrorSurgery
         (int stck[], int stack_top,
          int last_index, SecondaryRepairInfo repair)
{
    int stack_deletions = 0;
    Location  previous_loc = Loc(buffer[2]);

    for (int top = stack_top;
         top >= 0 && repair.num_deletions >= stack_deletions; top--)
    {
        if (location_stack[top] < previous_loc)
            stack_deletions++;
        previous_loc = location_stack[top];

        for (int i = 1;
             i <= last_index &&
                 repair.num_deletions >= (stack_deletions + i - 1); i++)
        {
            int j = ParseCheck(stck, top, lex_stream -> Kind(buffer[i]),
                               i + 1);

            if ((j - i + 1) > MIN_DISTANCE)
            {
                int k = stack_deletions + i - 1;
                if ((k < repair.num_deletions) ||
                    (j - k) > (repair.distance - repair.num_deletions))
                {
                    repair.code = DELETION_CODE;
                    repair.distance = j;
                    repair.stack_position = top;
                    repair.buffer_position = i;
                    repair.num_deletions = k;
                }
            }
        }
    }

    return repair;
}


/*****************************************************************/
/* Try to parse until first_token and all tokens in BUFFER have  */
/* been consumed, or an error is encountered. Return the number  */
/* of tokens that were expended before the parse blocked.        */
/*****************************************************************/
int Parser::ParseCheck(int stck[], int stack_top, int first_token,
                       int buffer_position)
{
    int max_pos,
        indx,
        ct,
        act,
        lhs_symbol;

/*****************************************************************/
/* Initialize pointer for temp_stack and initialize maximum      */
/* position of state stack that is still useful.                 */
/*****************************************************************/
    act = stck[stack_top];
    if (first_token > NT_OFFSET)
    {
        temp_stack_top = stack_top;
        max_pos = stack_top;
        indx = buffer_position;
        ct = lex_stream -> Kind(buffer[indx]);
        lex_stream -> Reset(lex_stream -> Next(buffer[indx]));
        lhs_symbol = first_token - NT_OFFSET;
        act = nt_action(act, lhs_symbol);
        if (act <= NUM_RULES)
            goto process_non_terminal;
    }
    else
    {
        temp_stack_top = stack_top - 1;
        max_pos = temp_stack_top;
        indx = buffer_position - 1;
        ct = first_token;
        lex_stream -> Reset(buffer[buffer_position]);
    }

 process_terminal:
    while (true)
    {
        if (++temp_stack_top >= stack_length)  /* Stack overflow!!! */
            return indx;
        temp_stack[temp_stack_top] = act;

        act = t_action(act, ct, lex_stream);

        if (act <= NUM_RULES)                   /* reduce action */
            temp_stack_top--;
        else if (act < ACCEPT_ACTION ||          /* shift action */
                 act > ERROR_ACTION)        /*shift-reduce action*/
        {
            if (indx == MAX_DISTANCE)
                return indx;
            indx++;
            ct = lex_stream -> Kind(buffer[indx]);
            lex_stream -> Reset(lex_stream -> Next(buffer[indx]));
            if (act > ERROR_ACTION)
                 act -= ERROR_ACTION;
            else goto process_terminal;
        }
        else if (act == ACCEPT_ACTION)           /* accept action */
             return MAX_DISTANCE;
        else return indx;                         /* error action */

        process_non_terminal:
        do
        {
            temp_stack_top -= (rhs[act]-1);
            lhs_symbol = lhs[act];
            act = (temp_stack_top > max_pos
                                  ? temp_stack[temp_stack_top]
                                  : stck[temp_stack_top]);
            act = nt_action(act, lhs_symbol);
        } while (act <= NUM_RULES);

        max_pos = Min(max_pos, temp_stack_top);
    } // process_terminal;

    return 0;
}


//
// Check if we're in a context where multi-catch might be valid.
// We're looking for the pattern: catch ( Type |
// The parse stack should have catch and ( tokens nearby.
//
bool Parser::IsMultiCatchContext()
{
    // Check if parse_stack has an AstType at or near the top
    if (state_stack_top < 2)
        return false;

    // Look for 'catch' keyword in recent tokens
    for (int i = state_stack_top; i >= 0 && i >= state_stack_top - 5; i--)
    {
        TokenObject tok = location_stack[i];
        if (lex_stream -> Kind(tok) == TK_catch)
            return true;
    }

    return false;
}


//
// Parse a type for multi-catch handling.
// This is a simplified type parser that handles:
// - Simple names (IOException)
// - Qualified names (java.io.IOException)
// Returns the parsed type, or NULL if parsing fails.
//
AstType* Parser::ParseType(TokenObject& curtok)
{
    if (lex_stream -> Kind(curtok) != TK_Identifier)
        return NULL;

    // Build a Name (possibly qualified)
    AstName* name = ast_pool -> NewName(curtok);
    curtok = lex_stream -> Gettoken(end_token);

    // Handle qualified names: a.b.c
    while (lex_stream -> Kind(curtok) == TK_DOT)
    {
        curtok = lex_stream -> Gettoken(end_token); // consume '.'
        if (lex_stream -> Kind(curtok) != TK_Identifier)
            return NULL;

        AstName* qualified = ast_pool -> NewName(curtok);
        qualified -> base_opt = name;
        name = qualified;
        curtok = lex_stream -> Gettoken(end_token);
    }

    // Convert Name to TypeName
    AstTypeName* type_name = ast_pool -> NewTypeName(name);
    return type_name;
}


//
// Try to handle multi-catch syntax: catch (Type1 | Type2 | ... identifier)
// Called when we hit an error and the current token is '|'.
// Returns true if we successfully handled multi-catch, false otherwise.
//
// IMPLEMENTATION NOTE:
// Multi-catch (Java 7) requires extending the parser grammar to accept:
//   CatchClause ::= 'catch' '(' CatchType Identifier ')' Block
//   CatchType ::= Type
//   CatchType ::= CatchType '|' Type
//
// The parser is table-driven (LPG/LR style) and the grammar tool (jikespg)
// that generated the tables is no longer available. Manually modifying the
// parser tables (javaprs.h, javadcl.h) to add these rules is complex because:
// 1. term_action and base_action tables encode state transitions
// 2. New rules need new rule numbers and actions
// 3. State machine consistency is hard to maintain manually
//
// Alternative approaches to consider:
// 1. Pre-process token stream to transform multi-catch into standard form
// 2. Use a separate parser for catch clauses with union types
// 3. Regenerate tables if a compatible grammar tool can be found
//
// The AST infrastructure for union types (AstUnionType) is in place.
// This function currently detects multi-catch syntax but cannot resume
// the LR parser correctly after manual parsing.
//
bool Parser::TryHandleMultiCatch(TokenObject& /* curtok */, int& /* current_kind */)
{
    // Multi-catch parsing is not yet implemented.
    // The AST types (AstUnionType) are ready, but the parser cannot be
    // easily extended without the original grammar tool.
    return false;
}


} // Close namespace Jopa block

