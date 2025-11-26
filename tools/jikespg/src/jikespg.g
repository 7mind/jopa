%Options nogoto-default,esc=$,os=100,action,actfile=lpgact.i,noem
%Options fp=lpg,gp,nodefer,suffix=_TK,stack_size=21,hactfile=lpgact.h
-- $Id: jikespg.g,v 1.2 1999/11/04 14:02:22 shields Exp $
-- This software is subject to the terms of the IBM Jikes Compiler
-- License Agreement available at the following URL:
-- http://www.ibm.com/research/jikes.
-- Copyright (C) 1983, 1999, International Business Machines Corporation
-- and others.  All Rights Reserved.
-- You must accept the terms of that agreement to use this software.

-- This grammar has been augmented with productions that captures
-- most errors that a user is likely to make. This saves the need
-- to have an error recovery system.
--
$Define ----------------------------------------------------------------

$offset /.    ./

$location
/.

/* $rule_text */
#line $next_line "$input_file"./

$action
/.act$rule_number, /* $rule_number */./

$null_action
/.$offset null_action, /* $rule_number */./

$no_action
/.$offset null_action, /* $rule_number */./

------------------------------------------------------------------------

$Terminals
    DEFINE_KEY TERMINALS_KEY ALIAS_KEY START_KEY RULES_KEY
    NAMES_KEY END_KEY

    EQUIVALENCE ARROW OR

    EMPTY_SYMBOL ERROR_SYMBOL EOL_SYMBOL EOF_SYMBOL

    MACRO_NAME SYMBOL BLOCK HBLOCK

    EOF

------------------------------------------------------------------------

$Alias

    '::=' ::= EQUIVALENCE
    '->'  ::= ARROW
    '|'   ::= OR
    $EOF  ::= EOF

------------------------------------------------------------------------

$Rules

/:static void (*rule_action[]) (void) = {NULL,:/

/.
#line $next_line "$input_file"
#define SYM1 terminal[stack_top + 1]
#define SYM2 terminal[stack_top + 2]
#define SYM3 terminal[stack_top + 3]

static void null_action(void)
{
    return;
}

static void add_macro_definition(char *name, struct terminal_type *term)
{
    if (num_defs >= (int)defelmt_size)
    {
        defelmt_size += DEFELMT_INCREMENT;
        defelmt = (struct defelmt_type *)
            (defelmt == (struct defelmt_type *) NULL
             ? malloc(defelmt_size * sizeof(struct defelmt_type))
             : realloc(defelmt, defelmt_size * sizeof(struct defelmt_type)));
        if (defelmt == (struct defelmt_type *) NULL)
            nospace(__FILE__, __LINE__);
    }

    defelmt[num_defs].length       = (*term).length;
    defelmt[num_defs].start_line   = (*term).start_line;
    defelmt[num_defs].start_column = (*term).start_column;
    defelmt[num_defs].end_line     = (*term).end_line;
    defelmt[num_defs].end_column   = (*term).end_column;
    strcpy(defelmt[num_defs].name, name);
    num_defs++;

    return;
}

static void add_block_definition(struct terminal_type *term)
{
    if (num_acts >= (int) actelmt_size)
    {
        actelmt_size += ACTELMT_INCREMENT;
        actelmt = (struct actelmt_type *)
            (actelmt == (struct actelmt_type *) NULL
             ? malloc(actelmt_size * sizeof(struct actelmt_type))
             : realloc(actelmt, actelmt_size * sizeof(struct actelmt_type)));
        if (actelmt == (struct actelmt_type *) NULL)
            nospace(__FILE__, __LINE__);
    }

    actelmt[num_acts].rule_number  = num_rules;
    actelmt[num_acts].start_line   = (*term).start_line;
    actelmt[num_acts].start_column = (*term).start_column;
    actelmt[num_acts].end_line     = (*term).end_line;
    actelmt[num_acts].end_column   = (*term).end_column;
    actelmt[num_acts].header_block = ((*term).kind == HBLOCK_TK);
    num_acts++;

    return;
}
./
    LPG_INPUT ::= [define_block]
                  [terminals_block]
                  [alias_block]
                  [start_block]
                  [rules_block]
                  [names_block]
                  [%END]
/:$no_action:/
                | bad_symbol
/:$no_action:/

    bad_symbol ::= EQUIVALENCE
/:$offset bad_first_symbol, /* $rule_number */:/
/.$location
static void bad_first_symbol(void)
{
    sprintf(msg_line,
            "First symbol: \"%s\" found in file is illegal. "
            "Line %d, column %d",
            SYM1.name, SYM1.start_line, SYM1.start_column);
    PRNTERR(msg_line);

    exit(12);
}
./
                 | ARROW
/:$offset bad_first_symbol, /* $rule_number */:/
                 | OR
/:$offset bad_first_symbol, /* $rule_number */:/
                 | EMPTY_SYMBOL
/:$offset bad_first_symbol, /* $rule_number */:/
                 | ERROR_SYMBOL
/:$offset bad_first_symbol, /* $rule_number */:/
                 | MACRO_NAME
/:$offset bad_first_symbol, /* $rule_number */:/
                 | SYMBOL
/:$offset bad_first_symbol, /* $rule_number */:/
                 | BLOCK
/:$offset $action:/
/.$location
static void act$rule_number(void)
{
    sprintf(msg_line,
            "Action block cannot be first object in file. "
            "Line %d, column %d",
            SYM1.start_line, SYM1.start_column);
    PRNTERR(msg_line);

    exit(12);
}
./

    define_block ::= DEFINE_KEY
/:$no_action:/
                   | DEFINE_KEY macro_list
/:$no_action:/

    macro_list ::= macro_name_symbol macro_block
/:$offset $action:/
/.$location
static void act$rule_number(void)
{
    if (action_bit)
        add_macro_definition(SYM1.name, &(SYM2));

    return;
}
./
                 | macro_list macro_name_symbol macro_block
/:$offset $action:/
/.$location
static void act$rule_number(void)
{
    if (action_bit)
        add_macro_definition(SYM2.name, &(SYM3));

    return;
}
./

    macro_name_symbol ::= MACRO_NAME
/:$no_action:/
                        | SYMBOL          -- Warning, Escape missing !
/:$offset $action:/
/.$location
static void act$rule_number(void)
{
    sprintf(msg_line,
            "Macro name \"%s\" does not start with the "
            "escape character. Line %d, column %d",
            SYM1.name, SYM1.start_line, SYM1.start_column);
    PRNTWNG(msg_line);

     return;
}
./
                        | '|'             -- No Good !!!
/:$offset bad_macro_name, /* $rule_number */:/
/.$location
static void bad_macro_name(void)
{
    sprintf(msg_line,
            "Reserved symbol cannot be used as macro name. "
            "Line %d, column %d",
            SYM1.start_line, SYM1.start_column);
    PRNTERR(msg_line);

    exit(12);
}
./
                        | EMPTY_SYMBOL    -- No good !!!
/:$offset bad_macro_name, /* $rule_number */:/
                        | ERROR_SYMBOL    -- No good !!!
/:$offset bad_macro_name, /* $rule_number */:/
                        | produces        -- No good !!!
/:$offset bad_macro_name, /* $rule_number */:/
                        | BLOCK           -- No good !!!
/:$offset $action:/
/.$location
static void act$rule_number(void)
{
    sprintf(msg_line,
            "Macro name not supplied for macro definition. "
            "Line %d, column %d",
            SYM1.start_line, SYM1.start_column);
    PRNTERR(msg_line);

    exit(12);
}
./
                        | DEFINE_KEY         -- No good !!!
/:$offset $action:/
/.$location
static void act$rule_number(void)
{
    sprintf(msg_line,
            "Macro keyword misplaced. Line %d, column %d",
            SYM1.start_line, SYM1.start_column);
    PRNTERR(msg_line);

    exit(12);
}
./

    macro_block ::= BLOCK
/:$no_action:/
                  | '|'            -- No Good !!!
/:$offset definition_expected, /* $rule_number */:/
/.$location
static void definition_expected(void)
{
    sprintf(msg_line,
            "Definition block expected where symbol found. "
            "Line %d, column %d",
            SYM1.start_line, SYM1.start_column);
    PRNTERR(msg_line);

    exit(12);
}
./
                  | EMPTY_SYMBOL   -- No good !!!
/:$offset definition_expected, /* $rule_number */:/
                  | ERROR_SYMBOL   -- No good !!!
/:$offset definition_expected, /* $rule_number */:/
                  | produces       -- No good !!!
/:$offset definition_expected, /* $rule_number */:/
                  | SYMBOL         -- No good !!!
/:$offset definition_expected, /* $rule_number */:/
                  | keyword        -- No good !!!
/:$offset definition_expected, /* $rule_number */:/
                  | END_KEY        -- No good !
/:$offset definition_expected, /* $rule_number */:/


    terminals_block ::= TERMINALS_KEY {terminal_symbol}
/:$no_action:/

    terminal_symbol ::= SYMBOL
/:$offset process_terminal, /* $rule_number */:/
/.$location
static void process_terminal(void)
{
    assign_symbol_no(SYM1.name, OMEGA);

    return;
}
./
                      | '|'
/:$offset process_terminal, /* $rule_number */:/
                      | produces
/:$offset process_terminal, /* $rule_number */:/
                      | DEFINE_KEY         -- No Good !!!
/:$offset bad_terminal, /* $rule_number */:/
/.$location
static void bad_terminal(void)
{
    sprintf(msg_line,
            "Keyword  has been misplaced in Terminal section."
            "  Line %d, column %d",
            SYM1.start_line, SYM1.start_column);
    PRNTERR(msg_line);

    exit(12);
}
./
                      | TERMINALS_KEY      -- No Good !!!
/:$offset bad_terminal, /* $rule_number */:/
                      | BLOCK           -- No good !!!
/:$offset $action:/
/.$location
static void act$rule_number(void)
{
    sprintf(msg_line,
            "Misplaced block found in TERMINALS section."
            "  Line %d, column %d",
            SYM1.start_line, SYM1.start_column);
    PRNTERR(msg_line);

    exit(12);
}
./

    alias_block ::= ALIAS_KEY {alias_definition}
/:$no_action:/

   alias_definition ::= alias_lhs produces alias_rhs
/:$offset $action:/
/.$location
static void act$rule_number(void)
{
    register int image;
    char tok_string[SYMBOL_SIZE + 1];

    switch(SYM3.kind)
    {
        case EMPTY_SYMBOL_TK:
            image = empty;
            break;

        case SYMBOL_TK:
            assign_symbol_no(SYM3.name, OMEGA);
            image = symbol_image(SYM3.name);
            break;

        case ERROR_SYMBOL_TK:
            if (error_image > num_terminals)
            {
                restore_symbol(tok_string, kerror);
                sprintf(msg_line,
                        "Illegal aliasing to %s prior to its "
                        "definition.  Line %d, column %d",
                        tok_string,
                        SYM3.start_line, SYM3.start_column);
                PRNTERR(msg_line);
                exit(12);
            }
            image = error_image;
            break;

        case EOF_SYMBOL_TK:
            if (eoft_image > num_terminals)
            {
                restore_symbol(tok_string, keoft);
                sprintf(msg_line,
                        "Illegal aliasing to %s prior to its "
                        "definition. Line %d, column %d",
                        tok_string,
                        SYM3.start_line, SYM3.start_column);
                PRNTERR(msg_line);
                exit(12);
            }
            image = eoft_image;
            break;

        case EOL_SYMBOL_TK:
            if (eolt_image == OMEGA)
            {
                sprintf(msg_line,
                        "Illegal aliasing to EOL prior to its "
                        "definition. Line %d, column %d",
                        SYM3.start_line, SYM3.start_column);
                PRNTERR(msg_line);
                exit(12);
            }
            image = eolt_image;
            break;

        default: /* if SYM3.kind == symbol */
            image = symbol_image(SYM3.name);
            break;
    }

    switch(SYM1.kind)
    {
        case SYMBOL_TK:
            if (symbol_image(SYM1.name) != OMEGA)
            {
                restore_symbol(tok_string, SYM1.name);
                sprintf(msg_line,
                        "Symbol %s was previously defined. "
                        "Line %d, column %d", tok_string,
                        SYM1.start_line, SYM1.start_column);
                PRNTERR(msg_line);
                exit(12);
            }
            assign_symbol_no(SYM1.name, image);
            break;

        case ERROR_SYMBOL_TK:
            if (error_image > num_terminals || ! error_maps_bit)
            {
                if (image == empty      || image == eolt_image ||
                    image == eoft_image || image > num_terminals)
                {
                    restore_symbol(tok_string, kerror);
                    sprintf(msg_line,
                            "Illegal alias for symbol %s. "
                            "Line %d, column %d.",
                            tok_string,
                            SYM1.start_line, SYM1.start_column);
                    PRNTERR(msg_line);
                    exit(12);
                }
                alias_map(kerror, image);
                error_image = image;
            }
            else
            {
                restore_symbol(tok_string, kerror);
                sprintf(msg_line,
                        "Symbol %s was previously defined. "
                        "Line %d, column %d",
                        tok_string,
                        SYM1.start_line, SYM1.start_column);
                PRNTERR(msg_line);
                exit(12);
            }
            break;

        case EOF_SYMBOL_TK:
            if (eoft_image > num_terminals)
            {
                if (image == empty       || image == eolt_image  ||
                    image == error_image || image > num_terminals)
                {
                    restore_symbol(tok_string, keoft);
                    sprintf(msg_line,
                            "Illegal alias for symbol %s. "
                            "Line %d, column %d.",
                            tok_string,
                            SYM1.start_line, SYM1.start_column);
                    PRNTERR(msg_line);
                    exit(12);
                }
                alias_map(keoft, image);
                eoft_image = image;
            }
            else
            {
                restore_symbol(tok_string, keoft);
                sprintf(msg_line,
                        "Symbol %s was previously defined. "
                        "Line %d, column %d",
                        tok_string,
                        SYM1.start_line, SYM1.start_column);
                PRNTERR(msg_line);
                exit(12);
            }
            break;

        default: /* if SYM1.kind == EOL_SYMBOL */
            if (eolt_image == OMEGA)
            {
                if (image == empty ||
                    image == eoft_image ||
                    image == error_image ||
                    image > num_terminals)
                {
                    sprintf(msg_line,
                            "Illegal alias for symbol EOL. "
                            "Line %d, column %d.",
                            SYM1.start_line, SYM1.start_column);
                    PRNTERR(msg_line);
                    exit(12);
                }
                eolt_image = image;
            }
            else
            {
                sprintf(msg_line,
                        "Symbol EOL was previously defined. "
                        "Line %d, column %d",
                        SYM1.start_line, SYM1.start_column);
                PRNTERR(msg_line);
                exit(12);
            }
            break;
    }

    return;
}
./
                       | bad_alias_lhs
/:$no_action:/
                       | alias_lhs bad_alias_rhs
/:$no_action:/
                       | alias_lhs produces bad_alias_rhs
/:$no_action:/

    alias_lhs ::= SYMBOL
/:$no_action:/
                | ERROR_SYMBOL
/:$no_action:/
                | EOL_SYMBOL
/:$no_action:/
                | EOF_SYMBOL
/:$no_action:/

    alias_rhs ::= SYMBOL
/:$no_action:/
                | ERROR_SYMBOL
/:$no_action:/
                | EOL_SYMBOL
/:$no_action:/
                | EOF_SYMBOL
/:$no_action:/
                | EMPTY_SYMBOL
/:$no_action:/
                | '|'
/:$no_action:/
                | produces
/:$no_action:/

    bad_alias_rhs ::= DEFINE_KEY
/:$offset bad_alias_rhs, /* $rule_number */:/
/.$location
static void bad_alias_rhs(void)
{
    sprintf(msg_line,
            "Misplaced keyword found in Alias section. "
            "Line %d, column %d",
            SYM1.start_line, SYM1.start_column);
    PRNTERR(msg_line);

    exit(12);
}
./
                    | TERMINALS_KEY
/:$offset bad_alias_rhs, /* $rule_number */:/
                    | ALIAS_KEY
/:$offset bad_alias_rhs, /* $rule_number */:/
                    | BLOCK
/:$offset $action:/
/.$location
static void act$rule_number(void)
{
    sprintf(msg_line,
            "Misplaced block found in Alias section. "
            "Line %d, column %d",
            SYM1.start_line, SYM1.start_column);
    PRNTERR(msg_line);

    exit(12);
}
./


    bad_alias_lhs ::= bad_alias_rhs
/:$no_action:/
                    | EMPTY_SYMBOL
/:$offset $action:/
/.$location
static void act$rule_number(void)
{
    sprintf(msg_line,
            "Empty symbol cannot be aliased. "
            "Line %d, column %d",
            SYM1.start_line, SYM1.start_column);
    PRNTERR(msg_line);

    exit(12);
}
./
                    | produces
/:$offset missing_quote, /* $rule_number */:/
/.$location
static void missing_quote(void)
{
    sprintf(msg_line,
            "Symbol must be quoted when used as a "
            "grammar symbol. Line %d, column %d",
            ormark,
            SYM1.start_line, SYM1.start_column);
    PRNTERR(msg_line);

    exit(12);
}
./
                    | '|'
/:$offset missing_quote, /* $rule_number */:/


    start_block ::= START_KEY {start_symbol}
/:$no_action:/

    start_symbol ::= SYMBOL
/:$offset $action:/
/.$location
/*********************************************************************/
/*********************************************************************/
static void act$rule_number(void)
{
    register struct node *q;

    assign_symbol_no(SYM1.name, OMEGA);
    q = Allocate_node();
    q -> value = symbol_image(SYM1.name);
    if (start_symbol_root == NULL)
        q -> next = q;
    else
    {
        q -> next = start_symbol_root -> next;
        start_symbol_root -> next = q;
    }
    start_symbol_root = q;
    num_rules++;
    num_items++;
    SHORT_CHECK(num_items);

    return;
}
./
                  | '|'            -- No Good !!!
/:$offset bad_start_symbol, /* $rule_number */:/
/.$location
static void bad_start_symbol(void)
{
    sprintf(msg_line,
            "Symbol cannot be used as Start symbol. "
            "Line %d, column %d",
            SYM1.start_line, SYM1.start_column);
    PRNTERR(msg_line);

    exit(12);
}
./
                  | EMPTY_SYMBOL   -- No good !!!
/:$offset bad_start_symbol, /* $rule_number */:/
                  | ERROR_SYMBOL   -- No good !!!
/:$offset bad_start_symbol, /* $rule_number */:/
                  | produces       -- No good !!!
/:$offset bad_start_symbol, /* $rule_number */:/
                  | BLOCK          -- No good !!!
/:$offset $action:/
/.$location
static void act$rule_number(void)
{
    sprintf(msg_line,
            "Misplaced block found in Start section. "
            "Line %d, column %d",
            SYM1.start_line, SYM1.start_column);
    PRNTERR(msg_line);

    exit(12);
}
./
                  | DEFINE_KEY        -- No good !!!
/:$offset misplaced_keyword_found_in_START_section, /* $rule_number */:/
/.$location
static void misplaced_keyword_found_in_START_section(void)
{
    sprintf(msg_line,
            "Misplaced keyword found in START section. "
            "Line %d, column %d",
            SYM1.start_line, SYM1.start_column);
    PRNTERR(msg_line);

    exit(12);
}
./
                  | TERMINALS_KEY     -- No good !!!
/:$offset misplaced_keyword_found_in_START_section, /* $rule_number */:/
                  | ALIAS_KEY         -- No good !!!
/:$offset misplaced_keyword_found_in_START_section, /* $rule_number */:/
                  | START_KEY         -- No good !!!
/:$offset misplaced_keyword_found_in_START_section, /* $rule_number */:/

    rules_block ::= RULES_KEY
/:$offset $action:/
/.$location
static void act$rule_number(void)
{
    register struct node *q;

    if (start_symbol_root == NULL)
    {
        q = Allocate_node();
        q -> value = empty;
        q -> next = q;
        start_symbol_root = q;
        num_rules = 0;                 /* One rule */
        num_items = 0;                 /* 0 items */
    }
    build_symno();

    return;
}
./
                  | RULES_KEY rule_list
/:$offset $action:/
/.$location
static void act$rule_number(void)
{
    build_symno();

    return;
}
./

    produces ::= '::='
/:$no_action:/
               | '->'
/:$no_action:/

    rule_list ::= {action_block} SYMBOL produces
/:$offset $action:/
/.$location
static void act$rule_number(void)
{
    register struct node *q;

    assign_symbol_no(SYM2.name, OMEGA);
    if (start_symbol_root == NULL)
    {
        q = Allocate_node();
        q -> value = symbol_image(SYM2.name);
        q -> next = q;

        start_symbol_root = q;

        num_rules = 1;
        num_items = 1;
    }

/*********************************************************************/
/* Since we don't know for sure how many start symbols we have, a    */
/* "while" loop is used to increment the size of rulehdr. However,   */
/* it is highly unlikely that this loop would ever execute more than */
/* once if the size of RULE_INCREMENT is reasonable.                 */
/*********************************************************************/
    while (num_rules >= (int)rulehdr_size)
    {
        rulehdr_size += RULEHDR_INCREMENT;
        rulehdr = (struct rulehdr_type *)
            (rulehdr == (struct rulehdr_type *) NULL
             ? malloc(rulehdr_size * sizeof(struct rulehdr_type))
             : realloc(rulehdr, rulehdr_size * sizeof(struct rulehdr_type)));
        if (rulehdr == (struct rulehdr_type *) NULL)
            nospace(__FILE__, __LINE__);
    }

    rulehdr[num_rules].sp = ((SYM3.kind == ARROW_TK) ? TRUE : FALSE);
    rulehdr[num_rules].lhs = symbol_image(SYM2.name);
    rulehdr[num_rules].rhs_root = NULL;

    return;
}
./

                | rule_list '|'
/:$offset $action:/
/.$location
static void act$rule_number(void)
{
    num_rules++;
    if (num_rules >= (int)rulehdr_size)
    {
        rulehdr_size += RULEHDR_INCREMENT;
        rulehdr = (struct rulehdr_type *)
            (rulehdr == (struct rulehdr_type *) NULL
             ? malloc(rulehdr_size * sizeof(struct rulehdr_type))
             : realloc(rulehdr, rulehdr_size * sizeof(struct rulehdr_type)));
        if (rulehdr == (struct rulehdr_type *) NULL)
            nospace(__FILE__, __LINE__);
    }
    rulehdr[num_rules].sp = rulehdr[num_rules - 1].sp;
    rulehdr[num_rules].lhs = OMEGA;
    rulehdr[num_rules].rhs_root = NULL;

    return;
}
./
                | rule_list SYMBOL produces
/:$offset $action:/
/.$location
static void act$rule_number(void)
{
    num_rules++;
    if (num_rules >= (int)rulehdr_size)
    {
        rulehdr_size += RULEHDR_INCREMENT;
        rulehdr = (struct rulehdr_type *)
            (rulehdr == (struct rulehdr_type *) NULL
             ? malloc(rulehdr_size * sizeof(struct rulehdr_type))
             : realloc(rulehdr, rulehdr_size * sizeof(struct rulehdr_type)));
        if (rulehdr == (struct rulehdr_type *) NULL)
            nospace(__FILE__, __LINE__);
    }
    rulehdr[num_rules].sp = ((SYM3.kind == ARROW_TK) ? TRUE : FALSE);
    assign_symbol_no(SYM2.name, OMEGA);
    rulehdr[num_rules].lhs = symbol_image(SYM2.name);
    rulehdr[num_rules].rhs_root = NULL;

    return;
}
./

                | rule_list EMPTY_SYMBOL
/:$no_action:/
                | rule_list action_block
/:$no_action:/
                | rule_list ERROR_SYMBOL
/:$offset $action:/
/.$location
static void act$rule_number(void)
{
    register struct node *q;
    char tok_string[SYMBOL_SIZE + 1];

    if (error_image == DEFAULT_SYMBOL)
    {
        restore_symbol(tok_string, kerror);
        sprintf(msg_line,
                "%s not declared or aliased to terminal "
                "symbol. Line %d, column %d",
                tok_string,
                SYM2.start_line, SYM2.start_column);
        PRNTERR(msg_line);
        exit(12);
    }
    q = Allocate_node();
    q -> value = error_image;
    num_items++;
    SHORT_CHECK(num_items);
    if (rulehdr[num_rules].rhs_root == NULL)
        q -> next = q;
    else
    {
        q -> next = rulehdr[num_rules].rhs_root -> next;
         rulehdr[num_rules].rhs_root -> next = q;
    }
    rulehdr[num_rules].rhs_root = q;

    return;
}
./
                | rule_list SYMBOL
/:$offset $action:/
/.$location
static void act$rule_number(void)
{
    register int sym;
    register struct node *q;

    assign_symbol_no(SYM2.name, OMEGA);
    sym = symbol_image(SYM2.name);
    if (sym != empty)
    {
        if (sym == eoft_image)
        {
            sprintf(msg_line,
                    "End-of-file symbol cannot be used "
                    "in rule. Line %d, column %d",
                    SYM2.start_line, SYM2.start_column);
            PRNTERR(msg_line);
            exit(12);
        }
        q = Allocate_node();
        q -> value = sym;
        num_items++;
        SHORT_CHECK(num_items);
        if (rulehdr[num_rules].rhs_root == NULL)
            q -> next = q;
        else
        {
            q -> next = rulehdr[num_rules].rhs_root -> next;
            rulehdr[num_rules].rhs_root -> next = q;
        }
        rulehdr[num_rules].rhs_root = q;
    }

    return;
}
./
                | '|'                    -- can't be first SYMBOL
/:$offset bad_first_symbol_in_RULES_section, /* $rule_number */:/
/.$location
static void bad_first_symbol_in_RULES_section(void)
{
    sprintf(msg_line,
            "First symbol in Rules section is not "
            "a valid left-hand side.\n Line %d, column %d",
            SYM1.start_line, SYM1.start_column);
    PRNTERR(msg_line);

    exit(12);
}
./
                | EMPTY_SYMBOL           -- can't be first SYMBOL
/:$offset bad_first_symbol_in_RULES_section, /* $rule_number */:/
                | ERROR_SYMBOL           -- can't be first SYMBOL
/:$offset bad_first_symbol_in_RULES_section, /* $rule_number */:/
                | keyword                -- keyword out of place
/:$offset bad_first_symbol_in_RULES_section, /* $rule_number */:/
                | rule_list '|' produces            -- No good !!!
/:$offset rule_without_left_hand_side, /* $rule_number */:/
/.$location
static void rule_without_left_hand_side(void)
{
    sprintf(msg_line,
            "Rule without left-hand-side.  Line %d, column %d",
            SYM3.start_line, SYM3.start_column);
    PRNTERR(msg_line);

    exit(12);
}
./
                | rule_list action_block produces          -- No good !!!
/:$offset rule_without_left_hand_side, /* $rule_number */:/
                | rule_list EMPTY_SYMBOL produces   -- No good !!!
/:$offset rule_without_left_hand_side, /* $rule_number */:/
                | rule_list keyword produces        -- No good !!!
/:$offset $action:/
/.$location
static void act$rule_number(void)
{
    sprintf(msg_line,
            "Misplaced keyword found in Rules section "
            "Line %d, column %d",
            SYM2.start_line, SYM2.start_column);
    PRNTERR(msg_line);

    exit(12);
}
./

    action_block ::= BLOCK
/:$offset $action:/
/.$location
static void act$rule_number(void)
{
    if (action_bit)
        add_block_definition(&(SYM1));

    return;
}
./
                   | HBLOCK
/:$offset $action:/
/.$location
static void act$rule_number(void)
{
    if (action_bit)
        add_block_definition(&(SYM1));

    return;
}
./

    keyword ::= DEFINE_KEY
/:$offset misplaced_keyword_found_in_RULES_section, /* $rule_number */:/
/.$location
static void misplaced_keyword_found_in_RULES_section(void)
{
    sprintf(msg_line,
            "Misplaced keyword found in RULES section. "
            "Line %d, column %d",
            SYM1.start_line, SYM1.start_column);
    PRNTERR(msg_line);

    exit(12);
}
./
              | TERMINALS_KEY
/:$offset misplaced_keyword_found_in_RULES_section, /* $rule_number */:/
              | ALIAS_KEY
/:$offset misplaced_keyword_found_in_RULES_section, /* $rule_number */:/
              | START_KEY
/:$offset misplaced_keyword_found_in_RULES_section, /* $rule_number */:/
              | RULES_KEY
/:$offset misplaced_keyword_found_in_RULES_section, /* $rule_number */:/

    names_block ::= NAMES_KEY {names_definition}
/:$no_action:/

    names_definition ::= name produces name
/:$offset $action:/
/.$location
static void act$rule_number(void)
{
    if (error_maps_bit)
    {
        int symbol;

        switch(SYM1.kind)
        {
            case EMPTY_SYMBOL_TK:
                symbol = empty;
                break;

            case ERROR_SYMBOL_TK:
                symbol = error_image;
                break;

            case EOL_SYMBOL_TK:
                symbol = eolt_image;
                break;

            case EOF_SYMBOL_TK:
                symbol = eoft_image;
                break;

            default:
                symbol = symbol_image(SYM1.name);
                break;
        }

        if (symbol == OMEGA)
        {
            sprintf(msg_line,
                    "Symbol %s is undefined. Line %d, column %d",
            SYM1.name, SYM1.start_line, SYM1.start_column);
            PRNTERR(msg_line);

            exit(12);
        }

        if (symno[symbol].name_index != OMEGA)
        {
            sprintf(msg_line,
                    "Symbol %s has been named more than once. "
                    "Line %d, column %d.",
            SYM1.name, SYM1.start_line, SYM1.start_column);
            PRNTERR(msg_line);

            exit(12);
        }
         symno[symbol].name_index = name_map(SYM3.name);
     }

     return;
}
./
                       | bad_name produces name
/:$no_action:/
                       | name produces bad_name
/:$no_action:/

    name ::= SYMBOL
/:$no_action:/
           | EMPTY_SYMBOL
/:$no_action:/
           | ERROR_SYMBOL
/:$no_action:/
           | EOL_SYMBOL
/:$no_action:/
           | EOF_SYMBOL
/:$no_action:/
           | '|'
/:$no_action:/
           | produces
/:$no_action:/

      bad_name ::= DEFINE_KEY
/:$offset misplaced_keyword_found_in_NAMES_section, /* $rule_number */:/
/.$location
static void misplaced_keyword_found_in_NAMES_section(void)
{
    sprintf(msg_line,
            "Keyword  has been misplaced in NAMES section."
            "  Line %d, column %d",
            SYM1.start_line, SYM1.start_column);
    PRNTERR(msg_line);

    exit(12);
}
./
                 | TERMINALS_KEY
/:$offset misplaced_keyword_found_in_NAMES_section, /* $rule_number */:/
                 | ALIAS_KEY
/:$offset misplaced_keyword_found_in_NAMES_section, /* $rule_number */:/
                 | START_KEY
/:$offset misplaced_keyword_found_in_NAMES_section, /* $rule_number */:/
                 | RULES_KEY
/:$offset misplaced_keyword_found_in_NAMES_section, /* $rule_number */:/
                 | NAMES_KEY
/:$offset misplaced_keyword_found_in_NAMES_section, /* $rule_number */:/
                 | BLOCK
/:$offset $action:/
/.$location
static void act$rule_number(void)
{
    sprintf(msg_line,
            "Misplaced action block found in NAMES "
            "section. Line %d, column %d",
            SYM1.start_line, SYM1.start_column);
    PRNTERR(msg_line);

    exit(12);
}
./
                 | MACRO_NAME
/:$offset $action:/
/.$location
static void act$rule_number(void)
{
    sprintf(msg_line,
            "Misplaced macro name found in NAMES "
            "section. Line %d, column %d",
            SYM1.start_line, SYM1.start_column);
    PRNTERR(msg_line);

    exit(12);
}
./

------------------------------------------------------------------------

[define_block] ::= $EMPTY
/:$no_action:/
                 | define_block
/:$no_action:/

[terminals_block] ::= $EMPTY
/:$offset process_TERMINALS_section, /* $rule_number */:/
/.$location
static void process_TERMINALS_section(void)
{
    num_terminals = num_symbols;
    assign_symbol_no(keoft, OMEGA);
    eoft_image = symbol_image(keoft);

    if (error_maps_bit)
    {
        assign_symbol_no(kerror, OMEGA);
        error_image = symbol_image(kerror);
    }
    else error_image = DEFAULT_SYMBOL;   /* should be 0 */

    assign_symbol_no(kaccept, OMEGA);
    accept_image = symbol_image(kaccept);

    return;
}
./
                    | terminals_block
/:$offset process_TERMINALS_section, /* $rule_number */:/

[alias_block] ::= $EMPTY
/:$offset process_ALIAS_section, /* $rule_number */:/
/.$location
static void process_ALIAS_section(void)
{
    register int i,
                 k;
    register struct hash_type *p;

    k = 0;
    if (eoft_image <= num_terminals)
        k++;
    else
        num_terminals++;

    if (error_maps_bit)
    {
        if (error_image <= num_terminals)
            k++;
        else
        {
            num_terminals++;
            if (k == 1)
                error_image--;
        }
    }

    if (k > 0)
    {
        for (i = 0; i < HT_SIZE; i++)
        {
            p = hash_table[i];
            while(p != NULL)
            {
                if (p -> number > num_terminals)
                    p -> number -= k;
                else if (p -> number < -num_terminals)
                    p -> number += k;
                p = p -> link;
            }
        }
        num_symbols -= k;
        accept_image -= k;
    }
    if (eolt_image == OMEGA)
        eolt_image = eoft_image;
    if (error_image == DEFAULT_SYMBOL)
        alias_map(kerror, DEFAULT_SYMBOL);

    return;
}
./
                | alias_block
/:$offset process_ALIAS_section, /* $rule_number */:/

[start_block] ::= $EMPTY
/:$no_action:/
                | start_block
/:$no_action:/

[rules_block] ::= $EMPTY
/:$no_action:/
                | rules_block
/:$no_action:/

[names_block] ::= $EMPTY
/:$no_action:/
                | names_block
/:$no_action:/

[%END] ::= $EMPTY
/:$no_action:/
         | END_KEY
/:$no_action:/

------------------------------------------------------------------------

{terminal_symbol} ::= $EMPTY
/:$offset $action:/
/.$location
static void act$rule_number(void)
{
    assign_symbol_no(kempty, OMEGA);
    empty = symbol_image(kempty);

    return;
}
./
                    | {terminal_symbol} terminal_symbol
/:$no_action:/

{start_symbol} ::= $EMPTY
/:$no_action:/
                 | {start_symbol} start_symbol
/:$no_action:/

{alias_definition} ::= $EMPTY
/:$no_action:/
                     | {alias_definition} alias_definition
/:$no_action:/

{names_definition} ::= $EMPTY
/:$no_action:/
                     | {names_definition} names_definition
/:$no_action:/

{action_block} ::= $EMPTY
/:$no_action:/
                 | {action_block} action_block
/:$no_action:/

/:$offset NULL};:/
$End
