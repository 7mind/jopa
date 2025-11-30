#pragma once

#include "platform.h"
#include "ast.h"
#include "error.h"
#include "symbol.h"
#include "tuple.h"
#include "set.h"


namespace Jopa { // Open namespace Jopa block
class Control;
class TypeShadowSymbol;
class MethodShadowSymbol;
class CPClassInfo;
class ConstantPool;

//
// Maintains a stack of symbol tables, for storing the different variables
// which are in scope in a method.
//
class SymbolTableStack
{
public:
    void Push(SymbolTable* symtab) { table.Next() = symtab; }
    void Pop() { if (table.Length() > 0) table.Reset(table.Length() - 1); }
    int Size() { return table.Length(); }
    SymbolTable* Top()
    {
        return (SymbolTable*) (table.Length() > 0
                                ? table[table.Length() - 1] : NULL);
    }

    SymbolTable* operator[](const int i) { return table[i]; }

    //
    // Search for a variable in a stack of symbol tables starting at the
    // current symbol table and ending with the symbol table of the method
    // from which this call originates.
    //
    VariableSymbol* FindVariableSymbol(NameSymbol* name_symbol)
    {
        for (int i = static_cast<int>(table.Length()) - 1; i >= 0; i--)
        {
            VariableSymbol* symbol =
                table[i] -> FindVariableSymbol(name_symbol);
            if (symbol)
                return symbol;
        }
        return NULL;
    }

    //
    // Search for a type in a stack of symbol tables starting at the current
    // symbol table and ending with the symbol table of the method from which
    // this call originates.
    //
    TypeSymbol* FindTypeSymbol(NameSymbol* name_symbol)
    {
        for (int i = static_cast<int>(table.Length()) - 1; i >= 0; i--)
        {
            TypeSymbol* symbol = table[i] -> FindTypeSymbol(name_symbol);
            if (symbol)
                return symbol;
        }
        return NULL;
    }

    //
    // Search for a label in a stack of symbol tables starting at the current
    // symbol table and ending with the symbol table of the method from which
    // this call originates.
    //
    LabelSymbol* FindLabelSymbol(NameSymbol* name_symbol)
    {
        for (int i = static_cast<int>(table.Length()) - 1; i >= 0; i--)
        {
            LabelSymbol* label = table[i] -> FindLabelSymbol(name_symbol);
            if (label)
                return label;
        }
        return NULL;
    }

private:
    Tuple<SymbolTable*> table;
};


//
// Maintains a stack of information used by Semantic.
//
template <typename T>
class SemanticStack
{
public:
    void Push(T next = T()) { info.Next() = next; }
    void Pop()
    {
        if (info.Length())
            info.Reset(info.Length() - 1);
    }
    unsigned Size() { return info.Length(); }
    T Top()
    {
        return info.Length() ? info[info.Length() - 1] : T();
    }
    T operator[](const unsigned i) { return info[i]; }

private:
    Tuple<T> info;
};


//
// A stack of blocks, and their associated data.
//
class BlockStack
{
public:
    unsigned max_size;

    void Push(AstBlock* block_)
    {
        block.Next() = block_;
        index.Next() = 0;
        if (block.Length() > max_size)
            max_size = block.Length();
    }

    void Pop()
    {
        unsigned len = block.Length();
        if (len)
        {
            block.Reset(len - 1);
            index.Reset(len - 1);
        }
    }

    unsigned Size() { return block.Length(); }
    AstBlock* TopBlock()
    {
        return (AstBlock*) (block.Length() > 0
                            ? block[block.Length() - 1] : NULL);
    }

    AstBlock* operator[](const unsigned i) { return block[i]; }

    int& TopMaxEnclosedVariableIndex()
    {
        assert(index.Length());
        return index[index.Length() - 1];
    }

    BlockStack() : max_size(0) {}

private:
    Tuple<AstBlock*> block;
    Tuple<int> index;
};


//
// A stack of expressions where assignment occurred to a final variable.
//
class DefiniteFinalAssignmentStack
{
public:
    void Push() { info.Next().Reset(); }
    void Pop()
    {
        if (info.Length())
            info.Reset(info.Length() - 1);
    }
    unsigned Size() { return info.Length(); }
    Tuple<AstExpression*>& Top()
    {
        assert(info.Length());
        return info[info.Length() - 1];
    }

private:
    Tuple<Tuple<AstExpression*> > info;
};


//
// A stack of blocks, and related data for definite assignment analysis.
//
class DefiniteBlockStack
{
public:

    void Push(AstBlock* block_)
    {
        break_pairs[top_index].SetUniverse();
        continue_pairs[top_index].SetUniverse();

        block[top_index++] = block_;
    }

    void Pop()
    {
        if (top_index > 0)
            top_index--;
        else assert(false);
    }

    int Size() { return top_index; }
    AstBlock* Block(int i) { return block[i]; }
    AstBlock* TopBlock()
    {
        assert(top_index > 0);
        return block[top_index - 1];
    }

    DefinitePair& BreakPair(int i)
    {
        return break_pairs[i];
    }
    DefinitePair& ContinuePair(int i)
    {
        return continue_pairs[i];
    }

    DefinitePair& TopBreakPair()
    {
        assert(top_index > 0);
        return break_pairs[top_index - 1];
    }
    DefinitePair& TopContinuePair()
    {
        assert(top_index > 0);
        return continue_pairs[top_index - 1];
    }

    DefinitePair& ReturnPair()
    {
        return return_pair;
    }

    DefiniteBlockStack(int stack_size_, int set_size)
        : stack_size(stack_size_),
          top_index(0),
          return_pair(set_size, BitSet::UNIVERSE)
    {
        block = new AstBlock*[stack_size];
        break_pairs = new DefinitePair[stack_size];
        continue_pairs = new DefinitePair[stack_size];

        for (int i = 0; i < stack_size; i++)
        {
            break_pairs[i].Resize(set_size);
            continue_pairs[i].Resize(set_size);
        }
    }

    ~DefiniteBlockStack()
    {
        delete [] block;
        delete [] break_pairs;
        delete [] continue_pairs;
    }

private:
    int stack_size,
        top_index;
    AstBlock** block;

    DefinitePair* break_pairs;
    DefinitePair* continue_pairs;
    DefinitePair return_pair;
};


//
// This class stores the semantic environment associated with the current type
// being processed. A stack of these objects is maintained, so that when an
// inner or anonymous class is encountered, a new environment is used.
//
// This class can also be cloned. A local class must update constructors to
// add val$name shadows of final local variables, but may encounter the use of
// local variables after the constructor was encountered. So, the clone serves
// as a snapshot of the state in effect at the time the constructor is first
// called, to reuse that state when the constructor call is later updated.
//
class SemanticEnvironment
{
public:

    // The semantic processor, should be identical for all environments on the
    // state stack.
    Semantic* sem;
    SemanticEnvironment* previous; // Environment of the enclosing class, if any

    // The current class member being compiled.
    MethodSymbol* this_method;
    VariableSymbol* this_variable;
    AstStatement* explicit_constructor;

    // The arguments that need updating if local shadow variables are found.
    AstArguments* args;

    // Stacks used within the current method.
    SymbolTableStack symbol_table;
    SemanticStack<SymbolSet*> try_exception_table_stack;
    SemanticStack<AstTryStatement*> try_statement_stack;
    SemanticStack<AstBlock*> breakable_statement_stack;
    SemanticStack<AstBlock*> continuable_statement_stack;
    SemanticStack<unsigned> abrupt_finally_stack;
    BlockStack block_stack;

    //
    // This set of fields is used in definite assignment analysis; they should
    // not need to be cloned.
    //
    DefinitePair* definitely_assigned_variables;
    DefinitePair* universe;
    BitSet* blank_finals;
    BitSet* reachable_assignments;
    DefiniteBlockStack* definite_block_stack;
    DefiniteFinalAssignmentStack* definite_final_assignment_stack;
    Tuple<VariableSymbol*>* final_fields;
    bool processing_simple_assignment;

    SemanticEnvironment(Semantic* sem_, TypeSymbol* type_,
                        SemanticEnvironment* previous_ = NULL)
        : sem(sem_),
          previous(previous_),
          this_method(NULL),
          this_variable(NULL),
          explicit_constructor(NULL),
          args(NULL),
          definitely_assigned_variables(NULL),
          universe(NULL),
          blank_finals(NULL),
          reachable_assignments(NULL),
          definite_block_stack(NULL),
          definite_final_assignment_stack(NULL),
          final_fields(NULL),
          processing_simple_assignment(false),
          type(type_),
          next(NULL)
    {}


    ~SemanticEnvironment()
    {
        delete next; // if there was any clone, get rid of it
        // Clean up definite analysis data if not already cleaned
        delete final_fields;
        delete universe;
        delete definitely_assigned_variables;
        delete definite_final_assignment_stack;
        delete blank_finals;
        delete reachable_assignments;
    }

    //
    // Clone the immediate environment of "this" Semantic environment. The
    // immediate environment consists primarily of the stack of symbol tables
    // that are necessary for looking up local variables in the immediate
    // environment.
    //
    SemanticEnvironment* GetEnvironment(AstArguments* args)
    {
        SemanticEnvironment* clone = new SemanticEnvironment(sem, type);
        clone -> this_method = this_method;
        clone -> this_variable = this_variable;
        clone -> explicit_constructor = explicit_constructor;
        clone -> args = args;
        for (int i = 0; i < symbol_table.Size(); i++)
            clone -> symbol_table.Push(symbol_table[i]);
        clone -> next = next;
        next = clone;

        return clone;
    }

    TypeSymbol* Type() { return type; }

    //
    // Are we in a region with no current instance? This applies to all
    // static initializers, static methods, and explicit constructor
    // invocations, including initializers in interfaces.
    //
    inline bool StaticRegion()
    {
        return ((this_variable && this_variable -> ACC_STATIC()) ||
                (this_method && this_method -> ACC_STATIC()) ||
                explicit_constructor);
    }

private:

    TypeSymbol* type;
    SemanticEnvironment* next; // use to link an environment to its clones.
};


//
// A stack of semantic environments.
//
class SemanticEnvironmentStack
{
public:
    void Push(SemanticEnvironment* env) { info.Next() = env; }

    void Pop()
    {
        assert(info.Length());
        info.Reset(info.Length() - 1);
    }

    int Size() { return info.Length(); }

    SemanticEnvironment* Top()
    {
        assert(info.Length());
        return info[info.Length() - 1];
    }

    SemanticEnvironment* operator[](const int i) { return info[i]; }

private:
    Tuple<SemanticEnvironment*> info;
};


//
// A semantic object is associated with each compilation unit (ie. each .java
// file). It is subdivided into SemanticEnvironments, which detail the
// compilation state of the current class or interface.
//
class Semantic
{
public:
    Control& control; // The control object performing the compilation
    FileSymbol* source_file_symbol; // The source file name
    LexStream* lex_stream; // The source file contents
    AstCompilationUnit* compilation_unit; // The syntax tree
    DirectorySymbol* directory_symbol; // The source file location

    SymbolSet types_to_be_processed;

    SymbolSet referenced_package_imports;
    SymbolSet referenced_type_imports;

    // Flag to track if single-type imports have been fully processed
    bool imports_processed;

    int return_code;

    // The constructor
    Semantic(Control& control_, FileSymbol* file_symbol_)
        : control(control_),
          source_file_symbol(file_symbol_),
          lex_stream(file_symbol_ -> lex_stream),
          compilation_unit(file_symbol_ -> compilation_unit),
          directory_symbol(file_symbol_ -> directory_symbol),
          imports_processed(false),
          return_code(0),
          error(NULL),
          this_package(file_symbol_ -> package),
          processing_type(NULL)
    {
#ifdef JOPA_DEBUG
        int i;
        for (i = 0; i < Ast::_num_expr_or_stmt_kinds; i++)
        {
            ProcessExprOrStmt[i] = &Semantic::ProcessInvalidKind;
            DefiniteStmt[i] = &Semantic::DefiniteInvalidStatement;
        }
        for (i = 0; i < Ast::_num_expression_kinds; i++)
            DefiniteExpr[i] = &Semantic::DefiniteInvalidExpression;
        for (i = 0; i < AstPreUnaryExpression::_num_kinds; i++)
        {
            ProcessPreUnaryExpr[i] = NULL;
            DefinitePreUnaryExpr[i] = NULL;
        }
        for (i = 0; i < AstBinaryExpression::_num_kinds; i++)
        {
            ProcessBinaryExpr[i] = NULL;
            DefiniteBinaryExpr[i] = NULL;
        }
#endif // JOPA_DEBUG
        // Semantic processing of expressions.
        ProcessExprOrStmt[Ast::NAME] = &Semantic::ProcessName;
        ProcessExprOrStmt[Ast::DOT] = &Semantic::ProcessFieldAccess;
        ProcessExprOrStmt[Ast::INTEGER_LITERAL] =
            &Semantic::ProcessIntegerLiteral;
        ProcessExprOrStmt[Ast::LONG_LITERAL] = &Semantic::ProcessLongLiteral;
        ProcessExprOrStmt[Ast::FLOAT_LITERAL] = &Semantic::ProcessFloatLiteral;
        ProcessExprOrStmt[Ast::DOUBLE_LITERAL] =
            &Semantic::ProcessDoubleLiteral;
        ProcessExprOrStmt[Ast::TRUE_LITERAL] = &Semantic::ProcessTrueLiteral;
        ProcessExprOrStmt[Ast::FALSE_LITERAL] = &Semantic::ProcessFalseLiteral;
        ProcessExprOrStmt[Ast::STRING_LITERAL] =
            &Semantic::ProcessStringLiteral;
        ProcessExprOrStmt[Ast::CHARACTER_LITERAL] =
            &Semantic::ProcessCharacterLiteral;
        ProcessExprOrStmt[Ast::NULL_LITERAL] = &Semantic::ProcessNullLiteral;
        ProcessExprOrStmt[Ast::CLASS_LITERAL] =
            &Semantic::ProcessClassLiteral;
        ProcessExprOrStmt[Ast::ARRAY_ACCESS] = &Semantic::ProcessArrayAccess;
        ProcessExprOrStmt[Ast::CALL] = &Semantic::ProcessMethodInvocation;
        ProcessExprOrStmt[Ast::THIS_EXPRESSION] =
            &Semantic::ProcessThisExpression;
        ProcessExprOrStmt[Ast::SUPER_EXPRESSION] =
            &Semantic::ProcessSuperExpression;
        ProcessExprOrStmt[Ast::PARENTHESIZED_EXPRESSION] =
            &Semantic::ProcessParenthesizedExpression;
        ProcessExprOrStmt[Ast::CLASS_CREATION] =
            &Semantic::ProcessClassCreationExpression;
        ProcessExprOrStmt[Ast::ARRAY_CREATION] =
            &Semantic::ProcessArrayCreationExpression;
        ProcessExprOrStmt[Ast::POST_UNARY] =
            &Semantic::ProcessPostUnaryExpression;
        ProcessExprOrStmt[Ast::PRE_UNARY] =
            &Semantic::ProcessPreUnaryExpression;
        ProcessExprOrStmt[Ast::CAST] = &Semantic::ProcessCastExpression;
        ProcessExprOrStmt[Ast::BINARY] = &Semantic::ProcessBinaryExpression;
        ProcessExprOrStmt[Ast::INSTANCEOF] =
            &Semantic::ProcessInstanceofExpression;
        ProcessExprOrStmt[Ast::CONDITIONAL] =
            &Semantic::ProcessConditionalExpression;
        ProcessExprOrStmt[Ast::ASSIGNMENT] =
            &Semantic::ProcessAssignmentExpression;
        // Semantic processing of statements.
        ProcessExprOrStmt[Ast::LOCAL_CLASS] =
            &Semantic::ProcessClassDeclaration;
        ProcessExprOrStmt[Ast::THIS_CALL] = &Semantic::ProcessInvalidKind;
        ProcessExprOrStmt[Ast::SUPER_CALL] = &Semantic::ProcessInvalidKind;
        ProcessExprOrStmt[Ast::BLOCK] = &Semantic::ProcessBlock;
        ProcessExprOrStmt[Ast::LOCAL_VARIABLE_DECLARATION] =
            &Semantic::ProcessLocalVariableStatement;
        ProcessExprOrStmt[Ast::IF] = &Semantic::ProcessIfStatement;
        ProcessExprOrStmt[Ast::EMPTY_STATEMENT] =
            &Semantic::ProcessEmptyStatement;
        ProcessExprOrStmt[Ast::EXPRESSION_STATEMENT] =
            &Semantic::ProcessExpressionStatement;
        ProcessExprOrStmt[Ast::SWITCH] = &Semantic::ProcessSwitchStatement;
        ProcessExprOrStmt[Ast::WHILE] = &Semantic::ProcessWhileStatement;
        ProcessExprOrStmt[Ast::DO] = &Semantic::ProcessDoStatement;
        ProcessExprOrStmt[Ast::FOR] = &Semantic::ProcessForStatement;
        ProcessExprOrStmt[Ast::FOREACH] = &Semantic::ProcessForeachStatement;
        ProcessExprOrStmt[Ast::BREAK] = &Semantic::ProcessBreakStatement;
        ProcessExprOrStmt[Ast::CONTINUE] = &Semantic::ProcessContinueStatement;
        ProcessExprOrStmt[Ast::RETURN] = &Semantic::ProcessReturnStatement;
        ProcessExprOrStmt[Ast::THROW] = &Semantic::ProcessThrowStatement;
        ProcessExprOrStmt[Ast::SYNCHRONIZED_STATEMENT] =
            &Semantic::ProcessSynchronizedStatement;
        ProcessExprOrStmt[Ast::ASSERT] = &Semantic::ProcessAssertStatement;
        ProcessExprOrStmt[Ast::TRY] = &Semantic::ProcessTryStatement;

        //
        // Check statements for definite assignment.
        //
        // This and super calls are not normally statements, so
        // DefiniteThisCall and DefiniteSuperCall should be invoked manually
        // from DefiniteConstructorBody rather than automatically by
        // DefiniteStatement. Therefore, they are diverted to the no-op
        // DefiniteDefaultStatement.
        //
        DefiniteStmt[Ast::THIS_CALL] = &Semantic::DefiniteDefaultStatement;
        DefiniteStmt[Ast::SUPER_CALL] = &Semantic::DefiniteDefaultStatement;
        DefiniteStmt[Ast::BLOCK] = &Semantic::DefiniteBlock;
        DefiniteStmt[Ast::LOCAL_CLASS] =
            &Semantic::DefiniteLocalClassStatement;
        DefiniteStmt[Ast::LOCAL_VARIABLE_DECLARATION] =
            &Semantic::DefiniteLocalVariableStatement;
        DefiniteStmt[Ast::IF] = &Semantic::DefiniteIfStatement;
        DefiniteStmt[Ast::EMPTY_STATEMENT] =
            &Semantic::DefiniteDefaultStatement;
        DefiniteStmt[Ast::EXPRESSION_STATEMENT] =
            &Semantic::DefiniteExpressionStatement;
        DefiniteStmt[Ast::SWITCH] = &Semantic::DefiniteSwitchStatement;
        DefiniteStmt[Ast::WHILE] = &Semantic::DefiniteWhileStatement;
        DefiniteStmt[Ast::DO] = &Semantic::DefiniteDoStatement;
        DefiniteStmt[Ast::FOR] = &Semantic::DefiniteForStatement;
        DefiniteStmt[Ast::FOREACH] = &Semantic::DefiniteForeachStatement;
        DefiniteStmt[Ast::BREAK] = &Semantic::DefiniteBreakStatement;
        DefiniteStmt[Ast::CONTINUE] = &Semantic::DefiniteContinueStatement;
        DefiniteStmt[Ast::RETURN] = &Semantic::DefiniteReturnStatement;
        DefiniteStmt[Ast::THROW] = &Semantic::DefiniteThrowStatement;
        DefiniteStmt[Ast::SYNCHRONIZED_STATEMENT] =
            &Semantic::DefiniteSynchronizedStatement;
        DefiniteStmt[Ast::ASSERT] = &Semantic::DefiniteAssertStatement;
        DefiniteStmt[Ast::TRY] = &Semantic::DefiniteTryStatement;

        DefiniteExpr[Ast::NAME] = &Semantic::DefiniteName;
        DefiniteExpr[Ast::DOT] = &Semantic::DefiniteFieldAccess;
        DefiniteExpr[Ast::ARRAY_ACCESS] = &Semantic::DefiniteArrayAccess;
        DefiniteExpr[Ast::CALL] = &Semantic::DefiniteMethodInvocation;
        DefiniteExpr[Ast::PARENTHESIZED_EXPRESSION] =
            &Semantic::DefiniteParenthesizedExpression;
        DefiniteExpr[Ast::CLASS_CREATION] =
            &Semantic::DefiniteClassCreationExpression;
        DefiniteExpr[Ast::ARRAY_CREATION] =
            &Semantic::DefiniteArrayCreationExpression;
        DefiniteExpr[Ast::POST_UNARY] = &Semantic::DefinitePostUnaryExpression;
        DefiniteExpr[Ast::PRE_UNARY] = &Semantic::DefinitePreUnaryExpression;
        DefiniteExpr[Ast::CAST] = &Semantic::DefiniteCastExpression;
        DefiniteExpr[Ast::BINARY] = &Semantic::DefiniteBinaryExpression;
        DefiniteExpr[Ast::INSTANCEOF] =
            &Semantic::DefiniteInstanceofExpression;
        DefiniteExpr[Ast::CONDITIONAL] =
            &Semantic::DefiniteConditionalExpression;
        DefiniteExpr[Ast::ASSIGNMENT] =
            &Semantic::DefiniteAssignmentExpression;
        DefiniteExpr[Ast::INTEGER_LITERAL] =
            &Semantic::DefiniteDefaultExpression;
        DefiniteExpr[Ast::LONG_LITERAL] = &Semantic::DefiniteDefaultExpression;
        DefiniteExpr[Ast::FLOAT_LITERAL] =
            &Semantic::DefiniteDefaultExpression;
        DefiniteExpr[Ast::DOUBLE_LITERAL] =
            &Semantic::DefiniteDefaultExpression;
        DefiniteExpr[Ast::TRUE_LITERAL] = &Semantic::DefiniteDefaultExpression;
        DefiniteExpr[Ast::FALSE_LITERAL] =
            &Semantic::DefiniteDefaultExpression;
        DefiniteExpr[Ast::STRING_LITERAL] =
            &Semantic::DefiniteDefaultExpression;
        DefiniteExpr[Ast::CHARACTER_LITERAL] =
            &Semantic::DefiniteDefaultExpression;
        DefiniteExpr[Ast::NULL_LITERAL] = &Semantic::DefiniteDefaultExpression;
        DefiniteExpr[Ast::CLASS_LITERAL] =
            &Semantic::DefiniteDefaultExpression;
        DefiniteExpr[Ast::THIS_EXPRESSION] =
            &Semantic::DefiniteDefaultExpression;
        DefiniteExpr[Ast::SUPER_EXPRESSION] =
            &Semantic::DefiniteDefaultExpression;

        DefiniteBinaryExpr[AstBinaryExpression::PLUS] =
            &Semantic::DefiniteDefaultBinaryExpression;
        DefiniteBinaryExpr[AstBinaryExpression::LEFT_SHIFT] =
            &Semantic::DefiniteDefaultBinaryExpression;
        DefiniteBinaryExpr[AstBinaryExpression::RIGHT_SHIFT] =
            &Semantic::DefiniteDefaultBinaryExpression;
        DefiniteBinaryExpr[AstBinaryExpression::UNSIGNED_RIGHT_SHIFT] =
            &Semantic::DefiniteDefaultBinaryExpression;
        DefiniteBinaryExpr[AstBinaryExpression::LESS] =
            &Semantic::DefiniteDefaultBinaryExpression;
        DefiniteBinaryExpr[AstBinaryExpression::GREATER] =
            &Semantic::DefiniteDefaultBinaryExpression;
        DefiniteBinaryExpr[AstBinaryExpression::LESS_EQUAL] =
            &Semantic::DefiniteDefaultBinaryExpression;
        DefiniteBinaryExpr[AstBinaryExpression::GREATER_EQUAL] =
            &Semantic::DefiniteDefaultBinaryExpression;
        DefiniteBinaryExpr[AstBinaryExpression::AND] =
            &Semantic::DefiniteDefaultBinaryExpression;
        DefiniteBinaryExpr[AstBinaryExpression::XOR] =
            &Semantic::DefiniteDefaultBinaryExpression;
        DefiniteBinaryExpr[AstBinaryExpression::IOR] =
            &Semantic::DefiniteDefaultBinaryExpression;
        DefiniteBinaryExpr[AstBinaryExpression::EQUAL_EQUAL] =
            &Semantic::DefiniteDefaultBinaryExpression;
        DefiniteBinaryExpr[AstBinaryExpression::NOT_EQUAL] =
            &Semantic::DefiniteDefaultBinaryExpression;
        DefiniteBinaryExpr[AstBinaryExpression::AND_AND] =
            &Semantic::DefiniteAND_AND;
        DefiniteBinaryExpr[AstBinaryExpression::OR_OR] =
            &Semantic::DefiniteOR_OR;
        DefiniteBinaryExpr[AstBinaryExpression::STAR] =
            &Semantic::DefiniteDefaultBinaryExpression;
        DefiniteBinaryExpr[AstBinaryExpression::MINUS] =
            &Semantic::DefiniteDefaultBinaryExpression;
        DefiniteBinaryExpr[AstBinaryExpression::SLASH] =
            &Semantic::DefiniteDefaultBinaryExpression;
        DefiniteBinaryExpr[AstBinaryExpression::MOD] =
            &Semantic::DefiniteDefaultBinaryExpression;

        DefinitePreUnaryExpr[AstPreUnaryExpression::PLUS] =
            &Semantic::DefiniteDefaultPreUnaryExpression;
        DefinitePreUnaryExpr[AstPreUnaryExpression::MINUS] =
            &Semantic::DefiniteDefaultPreUnaryExpression;
        DefinitePreUnaryExpr[AstPreUnaryExpression::TWIDDLE] =
            &Semantic::DefiniteDefaultPreUnaryExpression;
        DefinitePreUnaryExpr[AstPreUnaryExpression::NOT] =
            &Semantic::DefiniteNOT;
        DefinitePreUnaryExpr[AstPreUnaryExpression::PLUSPLUS] =
            &Semantic::DefinitePLUSPLUSOrMINUSMINUS;
        DefinitePreUnaryExpr[AstPreUnaryExpression::MINUSMINUS] =
            &Semantic::DefinitePLUSPLUSOrMINUSMINUS;

        ProcessBinaryExpr[AstBinaryExpression::PLUS] = &Semantic::ProcessPLUS;
        ProcessBinaryExpr[AstBinaryExpression::LEFT_SHIFT] =
            &Semantic::ProcessLEFT_SHIFT;
        ProcessBinaryExpr[AstBinaryExpression::RIGHT_SHIFT] =
            &Semantic::ProcessRIGHT_SHIFT;
        ProcessBinaryExpr[AstBinaryExpression::UNSIGNED_RIGHT_SHIFT] =
            &Semantic::ProcessUNSIGNED_RIGHT_SHIFT;
        ProcessBinaryExpr[AstBinaryExpression::LESS] = &Semantic::ProcessLESS;
        ProcessBinaryExpr[AstBinaryExpression::GREATER] =
            &Semantic::ProcessGREATER;
        ProcessBinaryExpr[AstBinaryExpression::LESS_EQUAL] =
            &Semantic::ProcessLESS_EQUAL;
        ProcessBinaryExpr[AstBinaryExpression::GREATER_EQUAL] =
            &Semantic::ProcessGREATER_EQUAL;
        ProcessBinaryExpr[AstBinaryExpression::AND] = &Semantic::ProcessAND;
        ProcessBinaryExpr[AstBinaryExpression::XOR] = &Semantic::ProcessXOR;
        ProcessBinaryExpr[AstBinaryExpression::IOR] = &Semantic::ProcessIOR;
        ProcessBinaryExpr[AstBinaryExpression::AND_AND] =
            &Semantic::ProcessAND_AND;
        ProcessBinaryExpr[AstBinaryExpression::OR_OR] =
            &Semantic::ProcessOR_OR;
        ProcessBinaryExpr[AstBinaryExpression::EQUAL_EQUAL] =
            &Semantic::ProcessEQUAL_EQUAL;
        ProcessBinaryExpr[AstBinaryExpression::NOT_EQUAL] =
            &Semantic::ProcessNOT_EQUAL;
        ProcessBinaryExpr[AstBinaryExpression::STAR] = &Semantic::ProcessSTAR;
        ProcessBinaryExpr[AstBinaryExpression::MINUS] =
            &Semantic::ProcessMINUS;
        ProcessBinaryExpr[AstBinaryExpression::SLASH] =
            &Semantic::ProcessSLASH;
        ProcessBinaryExpr[AstBinaryExpression::MOD] = &Semantic::ProcessMOD;

        ProcessPreUnaryExpr[AstPreUnaryExpression::PLUS] =
            &Semantic::ProcessPLUS;
        ProcessPreUnaryExpr[AstPreUnaryExpression::MINUS] =
            &Semantic::ProcessMINUS;
        ProcessPreUnaryExpr[AstPreUnaryExpression::TWIDDLE] =
            &Semantic::ProcessTWIDDLE;
        ProcessPreUnaryExpr[AstPreUnaryExpression::NOT] =
            &Semantic::ProcessNOT;
        ProcessPreUnaryExpr[AstPreUnaryExpression::PLUSPLUS] =
            &Semantic::ProcessPLUSPLUSOrMINUSMINUS;
        ProcessPreUnaryExpr[AstPreUnaryExpression::MINUSMINUS] =
            &Semantic::ProcessPLUSPLUSOrMINUSMINUS;
#ifdef JOPA_DEBUG
        for (i = 1; i < AstPreUnaryExpression::_num_kinds; i++)
        {
            assert(ProcessPreUnaryExpr[i]);
            assert(DefinitePreUnaryExpr[i]);
        }
        for (i = 1; i < AstBinaryExpression::_num_kinds; i++)
        {
            assert(ProcessBinaryExpr[i]);
            assert(DefiniteBinaryExpr[i]);
        }
#endif // JOPA_DEBUG
    }

    ~Semantic()
    {
        delete error;
        // Clean up static import info
        for (unsigned i = 0; i < single_static_imports.Length(); i++)
            delete single_static_imports[i];
    }

    // Report a multi-token semantic warning or error.
    void ReportSemError(SemanticError::SemanticErrorKind kind,
                        TokenIndex ltok, TokenIndex rtok,
                        const wchar_t* s1 = NULL, const wchar_t* s2 = NULL,
                        const wchar_t* s3 = NULL, const wchar_t* s4 = NULL,
                        const wchar_t* s5 = NULL, const wchar_t* s6 = NULL,
                        const wchar_t* s7 = NULL, const wchar_t* s8 = NULL,
                        const wchar_t* s9 = NULL)
    {
        if (! error)
            error = new SemanticError(control, source_file_symbol);
        error -> Report(kind, ltok, rtok, s1, s2, s3, s4, s5, s6, s7, s8, s9);
    }

    // Report a semantic warning or error on a syntax tree branch.
    void ReportSemError(SemanticError::SemanticErrorKind kind, Ast* ast,
                        const wchar_t* s1 = NULL, const wchar_t* s2 = NULL,
                        const wchar_t* s3 = NULL, const wchar_t* s4 = NULL,
                        const wchar_t* s5 = NULL, const wchar_t* s6 = NULL,
                        const wchar_t* s7 = NULL, const wchar_t* s8 = NULL,
                        const wchar_t* s9 = NULL)
    {
        if (! error)
            error = new SemanticError(control, source_file_symbol);
        error -> Report(kind, ast -> LeftToken(), ast -> RightToken(),
                        s1, s2, s3, s4, s5, s6, s7, s8, s9);
    }

    // Report a single-token semantic warning or error.
    void ReportSemError(SemanticError::SemanticErrorKind kind, TokenIndex tok,
                        const wchar_t* s1 = NULL, const wchar_t* s2 = NULL,
                        const wchar_t* s3 = NULL, const wchar_t* s4 = NULL,
                        const wchar_t* s5 = NULL, const wchar_t* s6 = NULL,
                        const wchar_t* s7 = NULL, const wchar_t* s8 = NULL,
                        const wchar_t* s9 = NULL)
    {
        if (! error)
            error = new SemanticError(control, source_file_symbol);
        error -> Report(kind, tok, tok, s1, s2, s3, s4, s5, s6, s7, s8, s9);
    }

    unsigned NumErrors() { return (error ? error -> num_errors : 0); }

    //
    // If we had a bad compilation unit, print the parser messages.
    // If semantic errors were detected print them too.
    // Set the return code. Implemented in error.cpp.
    //
    void PrintMessages();

    PackageSymbol* Package() { return this_package; }

    // Implemented in decl.cpp - performs first pass over .java file.
    void CheckPackage();
    void ProcessTypeNames();
    void ProcessImports();
    TypeSymbol* ReadType(FileSymbol*, PackageSymbol*, NameSymbol*, TokenIndex);

    // Implemented in init.cpp - determines values of final fields.
    void ComputeFinalValue(VariableSymbol*);

    // Implemented in class.cpp - reads in a .class file.
    TypeSymbol* ProcessSignature(TypeSymbol*, const char*&, TokenIndex);
    TypeSymbol* ReadTypeFromSignature(TypeSymbol*, const char*, int,
                                      TokenIndex);
    TypeSymbol* ProcessNestedType(TypeSymbol*, NameSymbol*, TokenIndex);

    // Implemented in expr.cpp - semantic checks of expressions
    bool IsConstantTrue(AstExpression* expr);
    bool IsConstantFalse(AstExpression* expr);

    // Java 5: Boxing/Unboxing utilities - implemented in decl.cpp
    TypeSymbol* GetWrapperType(const TypeSymbol*);
    TypeSymbol* GetPrimitiveType(const TypeSymbol*);
    bool IsBoxingConversion(TypeSymbol*, TypeSymbol*);
    bool IsUnboxingConversion(TypeSymbol*, TypeSymbol*);
    MethodSymbol* GetBoxingMethod(TypeSymbol*);
    MethodSymbol* GetUnboxingMethod(TypeSymbol*);

private:
    enum
    {
        INT_SHIFT_MASK = 0x1f,
        LONG_SHIFT_MASK = 0x3f
    };

    SemanticError* error;

    // Implemented in decl.cpp - clean up after parsing
    void CleanUp();
    void CleanUpType(TypeSymbol*);

    // Implemented in decl.cpp - process a .java file for declarations
    void ProcessTypeHeader(AstClassDeclaration*);
    void ProcessTypeHeader(AstEnumDeclaration*);
    void ProcessTypeHeader(AstInterfaceDeclaration*);
    void ProcessTypeHeader(AstAnnotationDeclaration*);
    TypeSymbol* ProcessTypeHeaders(AstClassBody*, TypeSymbol* = NULL);
    void ProcessSuperinterface(TypeSymbol*, AstTypeName*);
    void ProcessTypeParameters(TypeSymbol*, AstTypeParameters*);
    void ProcessMethodTypeParameters(MethodSymbol*, AstTypeParameters*);
    ParameterizedType* ProcessTypeArguments(TypeSymbol*, AstTypeArguments*);
    void GenerateBridgeMethods(TypeSymbol*);
    void ProcessConstructorMembers(AstClassBody*);
    void ProcessMethodMembers(AstClassBody*);
    void ProcessClassBodyForEffectiveJavaChecks(AstClassBody*);
    void CheckForSerializationMistakes(AstClassBody*);
    void ProcessFieldMembers(AstClassBody*);
    void ProcessEnumConstantMembers(AstClassBody*);
    void ProcessMembers(AstClassBody*);
    void CompleteSymbolTable(AstClassBody*);

    // Implemented in body.cpp - process method bodies and field initializers
    void ProcessExecutableBodies(AstClassBody*);

    friend class TypeSymbol;
    friend class VariableSymbol;

    // Used in the handling of imports - see decl.cpp
    Tuple<Symbol*> import_on_demand_packages;
    Tuple<TypeSymbol*> single_type_imports;

    // Java 5: Static imports - store import info, resolve lazily during lookup
    struct StaticImportInfo {
        TypeSymbol* type;
        NameSymbol* member_name;
    };
    Tuple<StaticImportInfo*> single_static_imports;
    Tuple<TypeSymbol*> static_on_demand_imports;  // Types for wildcard static imports

    //
    // Where am I?
    //
    PackageSymbol* this_package;

    // Track the type being processed during header phase (before state_stack is set up)
    TypeSymbol* processing_type;

    // Look at state associated with the current type
    bool InDeprecatedContext()
    {
        return ThisType() -> IsDeprecated() ||
            (ThisMethod() && ThisMethod() -> IsDeprecated()) ||
            (ThisVariable() && ThisVariable() -> IsDeprecated());
    }
    TypeSymbol* ThisType() {
        return state_stack.Size() ? state_stack.Top() -> Type() : processing_type;
    }
    MethodSymbol*& ThisMethod() {
        static MethodSymbol* null_method = NULL;
        return state_stack.Size() ? state_stack.Top() -> this_method : null_method;
    }
    VariableSymbol*& ThisVariable()
    {
        static VariableSymbol* null_variable = NULL;
        return state_stack.Size() ? state_stack.Top() -> this_variable : null_variable;
    }
    AstStatement*& ExplicitConstructorInvocation()
    {
        static AstStatement* null_statement = NULL;
        return state_stack.Size() ? state_stack.Top() -> explicit_constructor : null_statement;
    }
    SymbolTableStack& LocalSymbolTable()
    {
        assert(state_stack.Size()); // This should never be called during header processing
        return state_stack.Top() -> symbol_table;
    }
    SemanticStack<SymbolSet*>& TryExceptionTableStack()
    {
        return state_stack.Top() -> try_exception_table_stack;
    }
    SemanticStack<AstTryStatement*>& TryStatementStack()
    {
        return state_stack.Top() -> try_statement_stack;
    }
    SemanticStack<AstBlock*>& BreakableStatementStack()
    {
        return state_stack.Top() -> breakable_statement_stack;
    }
    SemanticStack<AstBlock*>& ContinuableStatementStack()
    {
        return state_stack.Top() -> continuable_statement_stack;
    }
    SemanticStack<unsigned>& AbruptFinallyStack()
    {
        return state_stack.Top() -> abrupt_finally_stack;
    }
    BlockStack& LocalBlockStack()
    {
        return state_stack.Top() -> block_stack;
    }
    SemanticEnvironment* GetEnvironment(AstArguments* ast)
    {
        return state_stack.Top() -> GetEnvironment(ast);
    }
    bool StaticRegion()
    {
        return state_stack.Top() -> StaticRegion();
    }

    DefinitePair*& DefinitelyAssignedVariables()
    {
        return state_stack.Top() -> definitely_assigned_variables;
    }
    DefinitePair*& Universe() { return state_stack.Top() -> universe; }
    BitSet*& BlankFinals() { return state_stack.Top() -> blank_finals; }
    BitSet*& ReachableAssignments()
    {
        return state_stack.Top() -> reachable_assignments;
    }
    DefiniteBlockStack*& DefiniteBlocks()
    {
        return state_stack.Top() -> definite_block_stack;
    }
    DefiniteFinalAssignmentStack*& DefiniteFinalAssignments()
    {
        return state_stack.Top() -> definite_final_assignment_stack;
    }
    Tuple<VariableSymbol*>*& FinalFields()
    {
        return state_stack.Top() -> final_fields;
    }
    bool& ProcessingSimpleAssignment()
    {
        return state_stack.Top() -> processing_simple_assignment;
    }

    // A stack to allow nested type processing
    SemanticEnvironmentStack state_stack;

    // Implemented in expr.cpp - semantic checks of expressions
    bool IsIntValueRepresentableInType(AstExpression*, const TypeSymbol*);

    // Implemented in decl.cpp - nested class processing
    void CheckNestedMembers(TypeSymbol*, AstClassBody*);
    void CheckNestedTypeDuplication(SemanticEnvironment*, TokenIndex);
    TypeSymbol* ProcessNestedTypeName(TypeSymbol*, AstDeclaredType*);
    TypeSymbol* FindTypeInShadow(TypeShadowSymbol*, TokenIndex);
    void ReportTypeInaccessible(TokenIndex, TokenIndex, TypeSymbol*);
    void ReportTypeInaccessible(Ast* ast, TypeSymbol* type)
    {
        ReportTypeInaccessible(ast -> LeftToken(), ast -> RightToken(), type);
    }
    TypeSymbol* GetBadNestedType(TypeSymbol*, TokenIndex);
    TypeSymbol* FindNestedType(TypeSymbol*, TokenIndex);
    TypeSymbol* MustFindNestedType(TypeSymbol*, AstName*);
    void ProcessImportQualifiedName(AstName*);
    void ProcessPackageOrType(AstName*);
    void ProcessTypeImportOnDemandDeclaration(AstImportDeclaration*);
    TypeSymbol* FindSimpleNameType(PackageSymbol*, TokenIndex);
    void ProcessSingleTypeImportDeclaration(AstImportDeclaration*);

    // Java 5: Static import processing
    void ProcessSingleStaticImportDeclaration(AstImportDeclaration*);
    void ProcessStaticImportOnDemandDeclaration(AstImportDeclaration*);

    // Implemented in modifier.cpp - process declaration modifiers
    AccessFlags ProcessModifiers(AstModifiers*, const wchar_t*, u2, u2 = 0);
    AccessFlags ProcessPackageModifiers(AstPackageDeclaration*);
    AccessFlags ProcessTopLevelTypeModifiers(AstDeclaredType*);
    AccessFlags ProcessNestedTypeModifiers(TypeSymbol*, AstDeclaredType*);
    AccessFlags ProcessLocalClassModifiers(AstDeclaredType*);
    AccessFlags ProcessFieldModifiers(AstFieldDeclaration*);
    AccessFlags ProcessLocalModifiers(AstLocalVariableStatement*);
    AccessFlags ProcessFormalModifiers(AstFormalParameter*);
    AccessFlags ProcessMethodModifiers(AstMethodDeclaration*);
    AccessFlags ProcessConstructorModifiers(AstConstructorDeclaration*);
    AccessFlags ProcessInterfaceFieldModifiers(AstFieldDeclaration*);
    AccessFlags ProcessInterfaceMethodModifiers(AstMethodDeclaration*);
    AccessFlags ProcessInitializerModifiers(AstInitializerDeclaration*);
    AccessFlags ProcessEnumConstantModifiers(AstEnumConstant*);

    // Implemented in decl.cpp - process declarations
    void AddDefaultConstructor(TypeSymbol*);
    void AddEnumSyntheticMethods(TypeSymbol*);
    void ProcessConstructorDeclaration(AstConstructorDeclaration*);
    void ProcessMethodDeclaration(AstMethodDeclaration*);
    void ProcessFieldDeclaration(AstFieldDeclaration*);
    void ProcessFormalParameters(BlockSymbol*, AstMethodDeclarator*);
    void CheckFieldDeclaration(AstFieldDeclaration*, AstVariableDeclaratorId*,
                               const AccessFlags&);
    void CheckFieldName(AstVariableDeclaratorId*, NameSymbol*, bool);
    TypeSymbol* ImportType(TokenIndex, NameSymbol*);
    TypeSymbol* FindPrimitiveType(AstPrimitiveType*);
    TypeSymbol* FindType(TokenIndex);
    TypeSymbol* FindInaccessibleType(AstName*);
    TypeSymbol* MustFindType(AstName*);
    void ProcessType(AstType*);

    // Implemented in decl.cpp - process initializers
    void InitializeVariable(AstFieldDeclaration*, MethodSymbol*);
    void ProcessInitializer(AstInitializerDeclaration*, MethodSymbol*);
    void ProcessStaticInitializers(AstClassBody*);
    void ProcessInstanceInitializers(AstClassBody*);
    MethodSymbol* GetStaticInitializerMethod(unsigned estimate = 0);

    // Implemented in expr_ops.cpp - expression processing
    bool CanWideningPrimitiveConvert(const TypeSymbol*,
                                     const TypeSymbol*);
    bool CanNarrowingPrimitiveConvert(const TypeSymbol*,
                                      const TypeSymbol*);
    bool CanCastConvert(TypeSymbol*, TypeSymbol*, TokenIndex = 0);
    bool CanMethodInvocationConvert(const TypeSymbol*, const TypeSymbol*);
    bool CanSubtypeConvert(const TypeSymbol*, const TypeSymbol*);
    bool CanAssignmentConvert(const TypeSymbol*, AstExpression*);
    bool CanAssignmentConvertReference(const TypeSymbol*, const TypeSymbol*);
    LiteralValue* CastValue(const TypeSymbol*, AstExpression*);
    AstExpression* ConvertToType(AstExpression*, TypeSymbol*);
    AstExpression* PromoteUnaryNumericExpression(AstExpression*);
    void BinaryNumericPromotion(AstAssignmentExpression*);
    void BinaryNumericPromotion(AstBinaryExpression*);
    void BinaryNumericPromotion(AstConditionalExpression*);
    TypeSymbol* BinaryNumericPromotion(AstExpression*&, AstExpression*&);
    void MethodInvocationConversion(AstArguments*, MethodSymbol*);

    // Implemented in definite.cpp - definite (un)assignment analysis
    void (Semantic::*DefiniteStmt[Ast::_num_expr_or_stmt_kinds])(Ast*);
    inline void DefiniteStatement(Ast*);

    void DefiniteLoopBody(BitSet&);

    void DefiniteBlock(Ast*);
    void DefiniteLocalClassStatement(Ast*);
    void DefiniteLocalVariableStatement(Ast*);
    void DefiniteExpressionStatement(Ast*);
    void DefiniteSynchronizedStatement(Ast*);
    void DefiniteIfStatement(Ast*);
    void DefiniteWhileStatement(Ast*);
    void DefiniteForStatement(Ast*);
    void DefiniteForeachStatement(Ast*);
    void DefiniteSwitchStatement(Ast*);
    void DefiniteDoStatement(Ast*);
    void DefiniteBreakStatement(Ast*);
    void DefiniteContinueStatement(Ast*);
    void DefiniteReturnStatement(Ast*);
    void DefiniteThrowStatement(Ast*);
    void DefiniteTryStatement(Ast*);
    void DefiniteAssertStatement(Ast*);
    void DefiniteDefaultStatement(Ast*) {}
    void DefiniteThisCall(AstThisCall*);
    void DefiniteSuperCall(AstSuperCall*);
    void DefiniteInvalidStatement(Ast*) { assert(false); }
    DefiniteAssignmentSet* DefiniteInvalidExpression(AstExpression*,
                                                     DefinitePair&)
    {
        assert(false);
        return NULL;
    }
    void DefiniteInvalidExpression(Ast*) { assert(false); }

    VariableSymbol* DefiniteFinal(AstFieldAccess*);

    DefiniteAssignmentSet* (Semantic::*DefiniteExpr[Ast::_num_expression_kinds])(AstExpression*, DefinitePair&);
    DefiniteAssignmentSet* DefiniteName(AstExpression*, DefinitePair&);
    DefiniteAssignmentSet* DefiniteArrayAccess(AstExpression*,
                                               DefinitePair&);
    DefiniteAssignmentSet* DefiniteMethodInvocation(AstExpression*,
                                                    DefinitePair&);
    DefiniteAssignmentSet* DefiniteClassCreationExpression(AstExpression*,
                                                           DefinitePair&);
    DefiniteAssignmentSet* DefiniteArrayCreationExpression(AstExpression*,
                                                           DefinitePair&);
    DefiniteAssignmentSet* DefinitePreUnaryExpression(AstExpression*,
                                                      DefinitePair&);
    DefiniteAssignmentSet* DefinitePostUnaryExpression(AstExpression*,
                                                       DefinitePair&);
    DefiniteAssignmentSet* DefiniteBinaryExpression(AstExpression*,
                                                    DefinitePair&);
    DefiniteAssignmentSet* DefiniteInstanceofExpression(AstExpression*,
                                                        DefinitePair&);
    DefiniteAssignmentSet* DefiniteConditionalExpression(AstExpression*,
                                                         DefinitePair&);
    DefiniteAssignmentSet* DefiniteAssignmentExpression(AstExpression*,
                                                        DefinitePair&);
    DefiniteAssignmentSet* DefiniteDefaultExpression(AstExpression*,
                                                     DefinitePair&)
    {
        return NULL;
    }
    DefiniteAssignmentSet* DefiniteFieldAccess(AstExpression*,
                                               DefinitePair&);
    DefiniteAssignmentSet* DefiniteParenthesizedExpression(AstExpression*,
                                                           DefinitePair&);
    DefiniteAssignmentSet* DefiniteCastExpression(AstExpression*,
                                                  DefinitePair&);
    DefiniteAssignmentSet* DefiniteBooleanExpression(AstExpression*,
                                                     DefinitePair&);
    void DefiniteExpression(AstExpression*, DefinitePair&);

    DefiniteAssignmentSet* (Semantic::*DefinitePreUnaryExpr[AstPreUnaryExpression::_num_kinds])(AstExpression*, DefinitePair&);
    DefiniteAssignmentSet* DefiniteDefaultPreUnaryExpression(AstExpression*,
                                                             DefinitePair&);
    DefiniteAssignmentSet* DefiniteNOT(AstExpression*, DefinitePair&);
    DefiniteAssignmentSet* DefinitePLUSPLUSOrMINUSMINUS(AstExpression*,
                                                        DefinitePair&);

    DefiniteAssignmentSet* (Semantic::*DefiniteBinaryExpr[AstBinaryExpression::_num_kinds])(AstBinaryExpression*, DefinitePair&);
    DefiniteAssignmentSet* DefiniteDefaultBinaryExpression(AstBinaryExpression*,
                                                           DefinitePair&);
    DefiniteAssignmentSet* DefiniteAND_AND(AstBinaryExpression*,
                                           DefinitePair&);
    DefiniteAssignmentSet* DefiniteOR_OR(AstBinaryExpression*,
                                         DefinitePair&);

    void DefiniteArrayInitializer(AstArrayInitializer*, DefinitePair&);
    void DefiniteArrayInitializer(AstArrayInitializer*);
    void DefiniteVariableInitializer(AstVariableDeclarator*);
    void DefiniteBlockStatements(AstBlock*);
    void DefiniteMethodBody(AstMethodDeclaration*);
    void DefiniteConstructorBody(AstConstructorDeclaration*);
    void DefiniteBlockInitializer(AstBlock*, int);
    void DefiniteFieldInitializer(AstVariableDeclarator*);
    void DefiniteSetup();
    void DefiniteCleanUp();

    // Implemented in body.cpp - method bodies and statements
    void ProcessBlockStatements(AstBlock*);
    void ProcessThisCall(AstThisCall*);
    void ProcessSuperCall(AstSuperCall*);
    void WarnOfAccessibleFieldWithName(SemanticError::SemanticErrorKind,
                                       AstVariableDeclaratorId*, NameSymbol*,
                                       bool);
    void CheckThrow(AstTypeName*, Tuple<AstTypeName*>*);
    void ProcessMethodBody(AstMethodDeclaration*);
    void ProcessConstructorBody(AstConstructorDeclaration*);
    bool CheckedException(TypeSymbol*);
    bool UncaughtException(TypeSymbol*);
    const wchar_t* UncaughtExceptionContext();

    // Implemented in expr.cpp - expression processing
    wchar_t* Header(const NameSymbol*, AstArguments*);
    void ReportConstructorNotFound(Ast*, TypeSymbol*);
    void ReportMethodNotFound(AstMethodInvocation*, TypeSymbol*);
    MethodSymbol* FindConstructor(TypeSymbol*, Ast*, TokenIndex, TokenIndex);
    inline bool MoreSpecific(MethodSymbol*, MethodSymbol*);
    inline bool MoreSpecific(MethodSymbol*, Tuple<MethodSymbol*>&);
    inline bool NoMethodMoreSpecific(Tuple<MethodSymbol*>&, MethodSymbol*);
    inline bool MoreSpecific(MethodSymbol*, Tuple<MethodShadowSymbol*>&);
    inline bool NoMethodMoreSpecific(Tuple<MethodShadowSymbol*>&,
                                     MethodSymbol*);
    inline bool MethodApplicableByArity(MethodSymbol*, unsigned);
    void FindMethodInEnvironment(Tuple<MethodShadowSymbol*>&,
                                 SemanticEnvironment*&,
                                 SemanticEnvironment*, AstMethodInvocation*);
    MethodSymbol* FindMisspelledMethodName(TypeSymbol*,
                                           AstMethodInvocation*,
                                           NameSymbol*);
    MethodShadowSymbol* FindMethodInEnvironment(SemanticEnvironment*&,
                                                AstMethodInvocation*);
    MethodShadowSymbol* FindMethodInType(TypeSymbol*, AstMethodInvocation*,
                                         NameSymbol* = NULL);

    void ReportVariableNotFound(AstExpression*, TypeSymbol*);
    void FindVariableInEnvironment(Tuple<VariableSymbol*>&,
                                   SemanticEnvironment*&,
                                   SemanticEnvironment*, NameSymbol*,
                                   TokenIndex);
    VariableSymbol* FindMisspelledVariableName(TypeSymbol*,
                                               AstExpression*);
    VariableSymbol* FindVariableInEnvironment(SemanticEnvironment*&,
                                              TokenIndex);
    VariableSymbol* FindVariableInType(TypeSymbol*, AstExpression*,
                                       NameSymbol* = NULL);
    VariableSymbol* FindLocalVariable(VariableSymbol*, TypeSymbol*);
    AstExpression* FindEnclosingInstance(AstExpression*, TypeSymbol*, bool);
    AstExpression* CreateAccessToType(Ast*, TypeSymbol*);
    void CreateAccessToScopedVariable(AstName*, TypeSymbol*);
    void CreateAccessToScopedMethod(AstMethodInvocation*, TypeSymbol*);

    bool TypeAccessCheck(TypeSymbol*);
    bool ConstructorAccessCheck(MethodSymbol*, bool);
    bool MemberAccessCheck(TypeSymbol*, Symbol*, AstExpression* = NULL);
    bool ProtectedAccessCheck(TypeSymbol*);

    void (Semantic::*ProcessPreUnaryExpr[AstPreUnaryExpression::_num_kinds])(AstPreUnaryExpression*);
    void ProcessPLUS(AstPreUnaryExpression*);
    void ProcessMINUS(AstPreUnaryExpression*);
    void ProcessTWIDDLE(AstPreUnaryExpression*);
    void ProcessNOT(AstPreUnaryExpression*);
    void ProcessPLUSPLUSOrMINUSMINUS(AstPreUnaryExpression*);

    void (Semantic::*ProcessBinaryExpr[AstBinaryExpression::_num_kinds])(AstBinaryExpression*);
    void ProcessPLUS(AstBinaryExpression*);
    void ProcessShift(AstBinaryExpression*);
    void ProcessShiftCount(TypeSymbol*, AstExpression*);
    void ProcessLEFT_SHIFT(AstBinaryExpression*);
    void ProcessRIGHT_SHIFT(AstBinaryExpression*);
    void ProcessUNSIGNED_RIGHT_SHIFT(AstBinaryExpression*);
    void ProcessLESS(AstBinaryExpression*);
    void ProcessGREATER(AstBinaryExpression*);
    void ProcessLESS_EQUAL(AstBinaryExpression*);
    void ProcessGREATER_EQUAL(AstBinaryExpression*);
    void ProcessAND(AstBinaryExpression*);
    void ProcessXOR(AstBinaryExpression*);
    void ProcessIOR(AstBinaryExpression*);
    void ProcessAND_AND(AstBinaryExpression*);
    void ProcessOR_OR(AstBinaryExpression*);
    void ProcessEQUAL_EQUAL(AstBinaryExpression*);
    void ProcessNOT_EQUAL(AstBinaryExpression*);
    void ProcessSTAR(AstBinaryExpression*);
    void ProcessMINUS(AstBinaryExpression*);
    void ProcessSLASH(AstBinaryExpression*);
    void ProcessMOD(AstBinaryExpression*);

    MethodShadowSymbol* FindMethodMember(TypeSymbol*, AstMethodInvocation*);
    void ProcessMethodName(AstMethodInvocation*);

    //
    // An array of member methods, to dispatch the various expressions and
    // statements for processing.
    //
    void (Semantic::*ProcessExprOrStmt[Ast::_num_expr_or_stmt_kinds])(Ast*);

    inline void ProcessStatement(AstStatement* stmt)
    {
        (this ->* ProcessExprOrStmt[stmt -> kind])(stmt);
    }

    inline void ProcessExpression(AstExpression* expr)
    {
        if (expr -> symbol)
            // already processed, make sure it was compiler-generated
            assert(expr -> generated);
        else (this ->* ProcessExprOrStmt[expr -> kind])(expr);
    }
    void ProcessExpressionOrStringConstant(AstExpression* expr);

    // Implemented in body.cpp - statement processing
    void CheckForAssignmentUsedAsTruthValue(Ast*);
    void ProcessLocalVariableStatement(Ast*);
    void ProcessBlock(Ast*);
    void ProcessForStatement(Ast*);
    void ProcessForeachStatement(Ast*);
    void ProcessSwitchStatement(Ast*);
    void ProcessThrowStatement(Ast*);
    void ProcessTryStatement(Ast*);
    void ProcessExpressionStatement(Ast*);
    void ProcessSynchronizedStatement(Ast*);
    void ProcessIfStatement(Ast*);
    void ProcessWhileStatement(Ast*);
    void ProcessDoStatement(Ast*);
    void ProcessBreakStatement(Ast*);
    void ProcessContinueStatement(Ast*);
    void ProcessReturnStatement(Ast*);
    void ProcessAssertStatement(Ast*);
    void ProcessEmptyStatement(Ast*);
    void ProcessInvalidKind(Ast* ast)
    {
        assert(ast -> IsExplicitConstructorInvocation());
        AstStatement* statement = (AstStatement*) ast;
        statement -> can_complete_normally = statement -> is_reachable;
        ReportSemError(SemanticError::MISPLACED_EXPLICIT_CONSTRUCTOR,
                       statement -> LeftToken(),
                       statement -> RightToken());
    }

    TypeSymbol* GetLocalType(AstDeclaredType*);
    void ProcessClassDeclaration(Ast*);

    // Implemented in expr.cpp - expression processing
    void CheckSimpleName(AstName*, SemanticEnvironment* where_found);
    void ProcessName(Ast*);
    void FindVariableMember(TypeSymbol*, AstExpression*);
    void ProcessAmbiguousName(AstName*);
    void ProcessFieldAccess(Ast*);
    void ProcessIntegerLiteral(Ast*);
    void ProcessLongLiteral(Ast*);
    void ProcessFloatLiteral(Ast*);
    void ProcessDoubleLiteral(Ast*);
    void ProcessTrueLiteral(Ast*);
    void ProcessFalseLiteral(Ast*);
    void ProcessStringLiteral(Ast*);
    void ProcessCharacterLiteral(Ast*);
    void ProcessArrayAccess(Ast*);
    bool ProcessArguments(AstArguments*);
    void ProcessMethodInvocation(Ast*);
    void ProcessNullLiteral(Ast*);
    void ProcessClassLiteral(Ast*);
    void ProcessThisExpression(Ast*);
    void ProcessSuperExpression(Ast*);
    void ProcessParenthesizedExpression(Ast*);
    void UpdateLocalConstructors(TypeSymbol*);
    void GetAnonymousConstructor(AstClassCreationExpression*,
                                 TypeSymbol*);
    TypeSymbol* GetAnonymousType(AstClassCreationExpression*,
                                 TypeSymbol*);
    void ProcessClassCreationExpression(Ast*);
    void ProcessArrayCreationExpression(Ast*);
    void ProcessPostUnaryExpression(Ast*);
    void ProcessPreUnaryExpression(Ast*);
    void ProcessCastExpression(Ast*);
    void ProcessBinaryExpression(Ast*);
    void ProcessInstanceofExpression(Ast*);
    void ProcessConditionalExpression(Ast*);
    // Type inference for conditional expressions (JLS 15.25.3)
    TypeSymbol* FindLeastUpperBound(TypeSymbol* type1, TypeSymbol* type2);
    void ProcessAssignmentExpression(Ast*);

    void ProcessVariableInitializer(AstVariableDeclarator*);
    void ProcessArrayInitializer(AstArrayInitializer*, TypeSymbol*);

    // Implemented in decl.cpp - inheritance of declared members
    void CheckMethodOverride(MethodSymbol*, MethodSymbol*, TypeSymbol*);
    void AddInheritedTypes(TypeSymbol*, TypeSymbol*);
    void AddInheritedFields(TypeSymbol*, TypeSymbol*);
    void AddInheritedMethods(TypeSymbol*, TypeSymbol*, TokenIndex);
    void ComputeTypesClosure(TypeSymbol*, TokenIndex);
    void ComputeFieldsClosure(TypeSymbol*, TokenIndex);
    void ComputeMethodsClosure(TypeSymbol*, TokenIndex);

    // Implemented in class.cpp - reads in a .class file.
    TypeSymbol* RetrieveNestedTypes(TypeSymbol*, wchar_t*, TokenIndex);
    TypeSymbol* GetType(TypeSymbol*, CPClassInfo*, const ConstantPool&,
                        TokenIndex);
    void ProcessClassFile(TypeSymbol*, const char*, unsigned, TokenIndex);
    void ReadClassFile(TypeSymbol*, TokenIndex);

    // Implemented in depend.cpp - class dependence tracking.
    void AddDependence(TypeSymbol*, TypeSymbol*, bool = false);
};



} // Close namespace Jopa block

