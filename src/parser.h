#ifndef parser_INCLUDED
#define parser_INCLUDED

#include "platform.h"
#include "lpginput.h"


namespace Jopa { // Open namespace Jopa block
class StoragePool;
class Ast;
class AstListNode;
class AstStatement;
class AstBlock;
class AstName;
class AstType;
class AstTypeArguments;
class AstTypeName;
class AstModifiers;
class AstPackageDeclaration;
class AstCompilationUnit;
class AstClassBody;
class AstTypeParameters;
class AstMethodBody;

enum ParseErrorCode
{
    ERROR_CODE,
    BEFORE_CODE,
    INSERTION_CODE,
    INVALID_CODE,
    SUBSTITUTION_CODE,
    DELETION_CODE,
    MERGE_CODE,
    MISPLACED_CODE,
    SCOPE_CODE,
    MANUAL_CODE,
    SECONDARY_CODE,
    EOF_CODE
};

struct PrimaryRepairInfo
{
    ParseErrorCode code;

    int distance,
        buffer_position,
        misspell_index,
        symbol;
};

struct SecondaryRepairInfo
{
    ParseErrorCode code;
    int distance,
        buffer_position,
        stack_position,
        num_deletions,
        symbol;

    bool recovery_on_next_stack;
};

class Parser : public javaprs_table
{
public:

    Parser() : ast_pool(NULL),
               parse_header_only(false),
               parse_package_header_only(false),
               location_stack(NULL),
               parse_stack(NULL),
               stack_length(0),
               stack(NULL),
               temp_stack(NULL)
    {
        InitRuleAction();
        return;
    }

    ~Parser()
    {
        delete [] stack;
        delete [] location_stack;
        delete [] parse_stack;
        delete [] temp_stack;
    }

    AstPackageDeclaration* PackageHeaderParse(LexStream*, StoragePool*);
    AstCompilationUnit* HeaderParse(LexStream*, StoragePool* = NULL);
    bool InitializerParse(LexStream*, AstClassBody*);
    bool BodyParse(LexStream*, AstClassBody*);

protected:
    TokenObject buffer[BUFF_SIZE];
    TokenObject end_token;

    Ast* HeaderParse();
    bool Initializer(AstClassBody*);
    bool Body(AstClassBody*);
    AstMethodBody* ParseSegment(TokenObject);

#define HEADERS
#include "javaact.h"

    void (Parser::*rule_action[NUM_RULES + 1]) ();

    void InitRuleAction();

    //********************************************************************
    //
    // Given a rule of the form     A ::= x1 x2 ... xn     n > 0
    //
    // the function Token(i) yields the symbol xi, if xi is a terminal
    // or ti, if xi is a nonterminal that produced a string of the form
    // xi => ti w.
    //
    //********************************************************************
    inline TokenIndex Token(int i)
    {
        return location_stack[state_stack_top + (i - 1)];
    }

    //********************************************************************
    //
    // Given a rule of the form     A ::= x1 x2 ... xn     n > 0
    //
    // the function Sym(i) yields the AST subtree associated with symbol
    // xi. NOTE that if xi is a terminal, Sym(i) is undefined !
    //
    //********************************************************************
    inline Ast*& Sym(int i) { return parse_stack[state_stack_top + (i - 1)]; }

    //********************************************************************
    //
    // When a token is shifted, we also construct a null AST for
    // it.  This is necessary in case we encounter an error and need to
    // delete AST subtrees from the parse stack - those corresponding to
    // shifted tokens should also have a valid subtree.
    //
    //********************************************************************
    inline void TokenAction(TokenObject) { Sym(1) = NULL; }

    LexStream* lex_stream;

    StoragePool* ast_pool;
    StoragePool* body_pool;
    StoragePool* list_node_pool;

    AstListNode* free_list_nodes;
    AstListNode* AllocateListNode();
    void FreeCircularList(AstListNode*);

    bool parse_header_only,
         parse_package_header_only;

    //
    // LOCATION_STACK is a stack that is "parallel" to
    // (STATE_)STACK that is used to keep
    // track of the location of the first token on which an action
    // was executed in the corresponding state.
    //
    Location* location_stack;
    Ast** parse_stack;

    enum { STACK_INCREMENT = 256 };

    int stack_length;
    int state_stack_top;
    int* stack;

    int temp_stack_top;
    int* temp_stack;

    static inline int Min(int x, int y) { return x < y ? x : y; }
    static inline int Max(int x, int y) { return x > y ? x : y; }

    void AllocateErrorStacks();
    void ReallocateStacks();
    void ErrorRepair(TokenObject error_token);
    void RepairParse(TokenObject);
    SecondaryRepairInfo ErrorSurgery(int stack[],
                                     int stack_top,
                                     int last_index,
                                     SecondaryRepairInfo repair);
    int ParseCheck(int stack[], int stack_top, int first_token,
                   int buffer_position);
};


} // Close namespace Jopa block

#endif // parser_INCLUDED

