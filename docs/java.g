#ifndef HEADERS

#ifdef HAVE_JIKES_NAMESPACE
namespace Jikes { // Open namespace Jikes block
#endif

void Parser::InitRuleAction()
{
    rule_action[0] = &Parser::BadAction;
#else // HEADERS
    AstTypeName* MakeTypeArguments(int tokennum);
    AstType* MakeArrayType(int tokennum);
    AstName* MakeSimpleName(int tokennum);
    AstModifiers* MakeModifiers();
    AstTypeParameters* MakeTypeParameters(int tokennum);
    AstTypeArguments* MakeExplicitTypeArguments(int tokennum);
    AstBlock* MakeBlock(int tokennum);
    AstStatement* MakeSwitchBlockStatement(AstListNode* labels,
                                           AstListNode* statements = NULL);
    void MakeCastExpression(AstType* type, int tokennum);

    void BadAction();
    void NoAction();
    void NullAction();
    void SetSym1ToSym2();
    void StartList();
    void AddList2();
    void AddList3();
    void MakeTypeArguments();
    void MakeArrayType();
    void MakeCompilationUnit();
    void MakePackageDeclaration();
    void MakeImportDeclaration();
    void MakeModifier();
    void MakeAnnotation();
    void MakeArrayInitializer();
    void MakeClassDeclaration();
    void MakeClassBody();
    void MakeMethodDeclaration();
    void MakeFieldDeclaration();
    void MakeMethodHeader();
    void MakeMethodDeclarator();
    void MakeFormalParameter();
    void MakeInitializerDeclaration();
    void MakeConstructorDeclaration();
    void MakeQualifiedSuper();
    void MakeEnumDeclaration();
    void MakeEnumBody();
    void MakeInterfaceDeclaration();
    void MakeAnnotationTypeDeclaration();
    void MakeAnnotationTypeMemberDeclaration();
    void MakeLocalVariable();
    void MakeLabeledStatement();
    void MakeExpressionStatement();
    void MakeIfThenElseStatement();
    void MakeSwitchLabel();
    void MakeWhileStatement();
    void MakeForStatement();
    void MakeForeachStatement();
    void MakeAssertStatement();
    void MakeTryStatement();
    void MakeParenthesizedExpression();
    void MakeClassLiteral();
    void MakeQualifiedNew();
    void MakeArrayCreationUninitialized();
    void MakeArrayCreationInitialized();
    void MakeFieldAccess();
    void MakeMethodInvocation();
    void MakeArrayAccess();
    void MakePreUnaryExpression();
    void MakeCastExpression();
    void MakeBinaryExpression();
    void MakeInstanceofExpression();
    void MakeConditionalExpression();
    void MakeWildcard();
    void MakeTypeParameter();
    void MakeTypeBound();
#endif // HEADERS

:\

/.#line $next_line "$input_file"
// $Id: java.g,v 1.48 2004/03/25 13:32:27 ericb Exp $
// DO NOT MODIFY THIS FILE - it is generated using jikespg on java.g.
//
// This software is subject to the terms of the IBM Jikes Compiler Open
// Source License Agreement available at the following URL:
// http://ibm.com/developerworks/opensource/jikes.
// Copyright (C) 1996, 2004 IBM Corporation and others.  All Rights Reserved.
// You must accept the terms of that agreement to use this software.
//

#include "platform.h"
#include "parser.h"
#include "ast.h"
#include "stream.h"

#undef HEADERS
#include "javaact.h"

#ifdef HAVE_JIKES_NAMESPACE
namespace Jikes { // Open namespace Jikes block
#endif

//**************************************************************************//
//**************************************************************************//
//*                                                                        *//
//* Below, we show each rule of the Java grammar together with the         *//
//* semantic action that is invoked when the parser performs a reduction   *//
//* by that rule.                                                          *//
//**************************************************************************//
//**************************************************************************//

./

--18.2 Productions from 2.3: The syntactic Grammar

Goal ::= CompilationUnit
\:$NoAction:\
/.
//
// The parse was bad. Give up now.
//
void Parser::BadAction() { assert(false); }

$location
//
// Given a rule of the form A ::= x1 x2 ... xn        n >= 1
// Do nothing - Whatever Ast was produced for x1 is inherited by A.
//
void Parser::NoAction() {}
./

Goal ::= BodyMarker MethodBody
\:$SetSym1ToSym2:\
/.
//
// This next rule was added to allow the parser to recognize the body of a
// funtion (constructor, method, or initializer) out of context. Note that
// the artificial terminal BodyMarker is added here to prevent an ordinary
// parse from accepting a body as a valid input - i.e., to recognize a body
// out-of-context, the BodyMarker terminal must be inserted in front of the
// input stream containing the body in question.
$location
//
// Given a rule of the form A ::= x1 x2, inherit the result from x2.
//
void Parser::SetSym1ToSym2() { Sym(1) = Sym(2); }
./


--18.3 Productions from 3: Lexical Structure

Literal ::= IntegerLiteral
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    Sym(1) = ast_pool -> NewIntegerLiteral(Token(1));
}
./

Literal ::= LongLiteral
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    Sym(1) = ast_pool -> NewLongLiteral(Token(1));
}
./

Literal ::= FloatLiteral
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    Sym(1) = ast_pool -> NewFloatLiteral(Token(1));
}
./

Literal ::= DoubleLiteral
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    Sym(1) = ast_pool -> NewDoubleLiteral(Token(1));
}
./

Literal ::= BooleanLiteral
\:$NoAction:\
/.$shared_NoAction./

Literal ::= CharacterLiteral
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    Sym(1) = ast_pool -> NewCharacterLiteral(Token(1));
}
./

Literal ::= StringLiteral
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    Sym(1) = ast_pool -> NewStringLiteral(Token(1));
}
./

Literal ::= 'null'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    Sym(1) = ast_pool -> NewNullLiteral(Token(1));
}
./

BooleanLiteral ::= 'true'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    Sym(1) = ast_pool -> NewTrueLiteral(Token(1));
}
./

BooleanLiteral ::= 'false'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    Sym(1) = ast_pool -> NewFalseLiteral(Token(1));
}
./

--18.4 Productions from 4: Types, Values and Variables

Type ::= PrimitiveType
\:$NoAction:\
/.$shared_NoAction./

Type ::= ReferenceType
\:$NoAction:\
/.$shared_NoAction./

PrimitiveType ::= NumericType
\:$NoAction:\
/.$shared_NoAction./

PrimitiveType ::= 'boolean'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    Sym(1) = ast_pool -> NewPrimitiveType(Ast::BOOLEAN, Token(1));
}
./

NumericType ::= IntegralType
\:$NoAction:\
/.$shared_NoAction./

NumericType ::= FloatingPointType
\:$NoAction:\
/.$shared_NoAction./

IntegralType ::= 'byte'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    Sym(1) = ast_pool -> NewPrimitiveType(Ast::BYTE, Token(1));
}
./

IntegralType ::= 'short'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    Sym(1) = ast_pool -> NewPrimitiveType(Ast::SHORT, Token(1));
}
./

IntegralType ::= 'int'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    Sym(1) = ast_pool -> NewPrimitiveType(Ast::INT, Token(1));
}
./

IntegralType ::= 'long'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    Sym(1) = ast_pool -> NewPrimitiveType(Ast::LONG, Token(1));
}
./

IntegralType ::= 'char'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    Sym(1) = ast_pool -> NewPrimitiveType(Ast::CHAR, Token(1));
}
./

FloatingPointType ::= 'float'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    Sym(1) = ast_pool -> NewPrimitiveType(Ast::FLOAT, Token(1));
}
./

FloatingPointType ::= 'double'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    Sym(1) = ast_pool -> NewPrimitiveType(Ast::DOUBLE, Token(1));
}
./

--
-- Added rule to make parsing 'void' easier.
--
VoidType ::= 'void'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    Sym(1) = ast_pool -> NewPrimitiveType(Ast::VOID_TYPE, Token(1));
}
./

ReferenceType ::= ClassOrInterfaceType
\:$NoAction:\
/.$shared_NoAction./

ReferenceType ::= ArrayType
\:$NoAction:\
/.$shared_NoAction./

--
-- Simplify.
--
--ClassOrInterfaceType ::= ClassType
--ClassOrInterfaceType ::= InterfaceType
--ClassType ::= Name
--InterfaceType ::= Name
ClassOrInterfaceType ::= ClassOrInterface
\:$NoAction:\
/.$shared_NoAction./

--
-- Parameterized types were added in JSR 14.
-- Use of Marker allows us to easily find the closing '>'.
--
ClassOrInterfaceType ::= ClassOrInterface '<' TypeArgumentList1 Marker
\:$MakeTypeArguments:\
/.$location
void Parser::MakeTypeArguments() { Sym(1) = MakeTypeArguments(1); }

//
// Given AstName/AstType '<' TypeArgumentList1 at tokennum, generate the
// AstTypeName that includes the type arguments. There must be a production at
// tokennum + 3 to allow finding the closing '>'.
//
AstTypeName* Parser::MakeTypeArguments(int tokennum)
{
    AstTypeName* p = Sym(tokennum) -> NameCast()
        ? ast_pool -> NewTypeName(DYNAMIC_CAST<AstName*> (Sym(tokennum)))
        : DYNAMIC_CAST<AstTypeName*> (Sym(tokennum));
    AstListNode* tail = DYNAMIC_CAST<AstListNode*> (Sym(tokennum + 2));
    AstTypeArguments* q =
        ast_pool -> NewTypeArguments(Token(tokennum + 1),
                                     Token(tokennum + 3) - 1);
    q -> AllocateTypeArguments(tail -> index + 1);
    AstListNode* root = tail;
    do
    {
        root = root -> next;
        q -> AddTypeArgument(DYNAMIC_CAST<AstType*> (root -> element));
    } while (root != tail);
    FreeCircularList(tail);
    p -> type_arguments_opt = q;
    return p;
}
./

ClassOrInterface ::= Name
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstTypeName* p = ast_pool -> NewTypeName(DYNAMIC_CAST<AstName*> (Sym(1)));
    Sym(1) = p;
}
./

--
-- Parameterized types were added in JSR 14.
--
ClassOrInterface ::= ClassOrInterface '<' TypeArgumentList1 '.' Name
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstTypeName* p = ast_pool -> NewTypeName(DYNAMIC_CAST<AstName*> (Sym(5)));
    p -> base_opt = MakeTypeArguments(1);
    Sym(1) = p;
}
./

--
-- These rules have been rewritten to avoid some conflicts introduced
-- by adding the 1.1 features, and to simplify syntax tree generation.
--
-- JLS1 lists:
--ArrayType ::= PrimitiveType '[' ']'
--ArrayType ::= Name '[' ']'
--ArrayType ::= ArrayType '[' ']'
-- JLS2 lists:
--ArrayType ::= Type '[' ']'
--
ArrayType ::= PrimitiveType Dims
\:$MakeArrayType:\
/.$location
void Parser::MakeArrayType() { Sym(1) = MakeArrayType(1); }

//
// Used on variants of "Type Dimsopt". If this type has dimensions, make an
// array type; otherwise return the type name.
//
AstType* Parser::MakeArrayType(int tokennum)
{
    AstType* p = Sym(tokennum) -> NameCast()
        ? ast_pool -> NewTypeName(DYNAMIC_CAST<AstName*> (Sym(tokennum)))
        : DYNAMIC_CAST<AstType*> (Sym(tokennum));
    return ! Sym(tokennum + 1) ? p
        : ast_pool -> NewArrayType(p, (DYNAMIC_CAST<AstBrackets*>
                                       (Sym(tokennum + 1))));
}
./

--ArrayType ::= ClassOrInterfaceType Dims
ArrayType ::= Name Dims
\:$MakeArrayType:\
/.$shared_function
//
// void MakeArrayType();
//./

--
-- Parameterized types were added in JSR 14.
--
ArrayType ::= ClassOrInterface '<' TypeArgumentList1 '.' Name Dims
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstTypeName* p = ast_pool -> NewTypeName(DYNAMIC_CAST<AstName*> (Sym(5)));
    p -> base_opt = MakeTypeArguments(1);
    Sym(5) = p;
    Sym(1) = MakeArrayType(5);
}
./

--
-- Parameterized types were added in JSR 14.
--
ArrayType ::= ClassOrInterface '<' TypeArgumentList1 Dims
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    Sym(3) = MakeTypeArguments(1);
    Sym(1) = MakeArrayType(3);
}
./

--
-- Simplify the syntax tree.
--
--ClassType ::= ClassOrInterfaceType
--InterfaceType ::= ClassOrInterfaceType

--18.5 Productions from 6: Names

Name ::= 'Identifier'
\:$action:\
/.$location
void Parser::Act$rule_number() { MakeSimpleName(1); }

//
// Used on "Identifier", and sets the corresponding symbol to a simple name.
//
AstName* Parser::MakeSimpleName(int tokennum)
{
    AstName* name = ast_pool -> NewName(Token(tokennum));
    Sym(tokennum) = name;
    return name;
}
./

--
-- The use of Marker allows us to share code.
--
--Name ::= Name '.' 'Identifier'
Name ::= Name '.' Marker 'Identifier'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstName* p = ast_pool -> NewName(Token(4));
    p -> base_opt = DYNAMIC_CAST<AstName*> (Sym(1));
    Sym(1) = p;
}
./

--18.6 Productions from 7: Packages

--
-- Annotations were added in JSR 175. As a result, we must inline expand
-- PackageDeclaration vs. TypeDeclaration in order to resolve the ambiguity
-- between '@A' starting '@A package B;' vs. '@A B{}'.
--
--CompilationUnit ::= PackageDeclarationopt ImportDeclarationsopt
--                    TypeDeclarationsopt
CompilationUnit ::= PackageDeclaration ImportDeclarationsopt
                    TypeDeclarationsopt
\:$MakeCompilationUnit:\
/.$location
void Parser::MakeCompilationUnit()
{
    AstCompilationUnit* p = ast_pool -> NewCompilationUnit();
    p -> package_declaration_opt =
        DYNAMIC_CAST<AstPackageDeclaration*> (Sym(1));
    if (Sym(2))
    {
        AstListNode* tail = DYNAMIC_CAST<AstListNode*> (Sym(2));
        p -> AllocateImportDeclarations(tail -> index + 1);
        AstListNode* root = tail;
        do
        {
            root = root -> next;
            p -> AddImportDeclaration(DYNAMIC_CAST<AstImportDeclaration*>
                                      (root -> element));
        } while (root != tail);
        FreeCircularList(tail);
    }
    if (Sym(3))
    {
        AstListNode* tail = DYNAMIC_CAST<AstListNode*> (Sym(3));
        p -> AllocateTypeDeclarations(tail -> index + 1);
        AstListNode* root = tail;
        do
        {
            root = root -> next;
            p -> AddTypeDeclaration(DYNAMIC_CAST<AstDeclaredType*>
                                    (root -> element));
        } while (root != tail);
        FreeCircularList(tail);
    }
    Sym(1) = p;
}
./

--
-- The use of Marker allows us to share code.
--
CompilationUnit ::= Marker ImportDeclarations TypeDeclarationsopt
\:$MakeCompilationUnit:\
/.$shared_function
//
// void MakeCompilationUnit();
//./

--
-- See comments above why this is inline expanded.
--
CompilationUnit ::= TypeDeclarationsopt
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    Sym(3) = Sym(1);
    Sym(1) = NULL;
    Sym(2) = NULL;
    MakeCompilationUnit();
}
./

ImportDeclarations ::= ImportDeclaration
\:$StartList:\
/.$location
//
// This starts a list containing a single element.
// Note that the list is circular so as to preserve the order of the elements.
//
void Parser::StartList()
{
    AstListNode* p = AllocateListNode();
    p -> next = p;
    p -> element = Sym(1);
    p -> index = 0;
    Sym(1) = p;
}
./

ImportDeclarations ::= ImportDeclarations ImportDeclaration
\:$AddList2:\
/.$location
//
// This adds token 2 to an existing list.
// Note that the list is circular so as to preserve the order of the elements.
//
void Parser::AddList2()
{
    AstListNode* tail = DYNAMIC_CAST<AstListNode*> (Sym(1));
    AstListNode* p = AllocateListNode();
    p -> element = Sym(2);
    p -> index = tail -> index + 1;
    p -> next = tail -> next;
    tail -> next = p;
    Sym(1) = p;
}

//
// This adds token 3 to an existing list (thus, token 2 was a delimiter).
// Note that the list is circular so as to preserve the order of the elements.
//
void Parser::AddList3()
{
    AstListNode* tail = DYNAMIC_CAST<AstListNode*> (Sym(1));
    AstListNode* p = AllocateListNode();
    p -> element = Sym(3);
    p -> index = tail -> index + 1;
    p -> next = tail -> next;
    tail -> next = p;
    Sym(1) = p;
}
./

ImportDeclarationsopt ::= $empty
\:$NullAction:\
/.$location
//
// Given a rule of the form A ::= x1 x2 ... xn
//
// Construct a NULL Ast for A.
//
void Parser::NullAction() { Sym(1) = NULL; }
./

ImportDeclarationsopt ::= ImportDeclarations
\:$NoAction:\
/.$shared_NoAction./

TypeDeclarations ::= TypeDeclaration
\:$StartList:\
/.$shared_StartList./

TypeDeclarations ::= TypeDeclarations TypeDeclaration
\:$AddList2:\
/.$shared_AddList2./

TypeDeclarationsopt ::= $empty
\:$NullAction:\
/.$shared_NullAction./

TypeDeclarationsopt ::= TypeDeclarations
\:$NoAction:\
/.$shared_NoAction./

--
-- The use of Marker allows us to share code.
--
--PackageDeclaration ::= 'package' Name PackageHeaderMarker ';'
PackageDeclaration ::= Marker 'package' Name PackageHeaderMarker ';'
\:$MakePackageDeclaration:\
/.$location
void Parser::MakePackageDeclaration()
{
    AstPackageDeclaration* p = ast_pool -> NewPackageDeclaration();
    p -> modifiers_opt = MakeModifiers();
    p -> package_token = Token(2);
    p -> name = DYNAMIC_CAST<AstName*> (Sym(3));
    p -> semicolon_token = Token(5);
    Sym(1) = p;
}

//
// Converts the list at symbol 1 to AstModifiers.
//
AstModifiers* Parser::MakeModifiers()
{
    if (! Sym(1))
        return NULL;
    AstModifiers* p = ast_pool -> NewModifiers();
    AstListNode* tail = DYNAMIC_CAST<AstListNode*> (Sym(1));
    p -> AllocateModifiers(tail -> index + 1);
    AstListNode* root = tail;
    do
    {
        root = root -> next;
        if (root -> element -> ModifierKeywordCast())
        {
            AstModifierKeyword* mod = (AstModifierKeyword*) root -> element;
            p -> AddModifier(mod);
            if (lex_stream -> Kind(mod -> modifier_token) == TK_static)
                p -> static_token_opt = mod -> modifier_token;
        }
        else p -> AddModifier(DYNAMIC_CAST<AstAnnotation*> (root -> element));
    } while (root != tail);
    FreeCircularList(tail);
    return p;
}
./

--
-- Annotations were added in JSR 175. We must use Modifiers with a semantic
-- check that no modifier keywords appeared, because of the ambiguity between
-- '@A @B' starting '@A @B package C;' or '@A @B class C{}'.
--
PackageDeclaration ::= Modifiers 'package' Name PackageHeaderMarker ';'
\:$MakePackageDeclaration:\
/.$shared_function
//
// void MakePackageDeclaration();
//./

ImportDeclaration ::= SingleTypeImportDeclaration
\:$NoAction:\
/.$shared_NoAction./

ImportDeclaration ::= TypeImportOnDemandDeclaration
\:$NoAction:\
/.$shared_NoAction./

--
-- Static imports were added in JSR 201.
--
ImportDeclaration ::= SingleStaticImportDeclaration
\:$NoAction:\
/.$shared_NoAction./

--
-- Static imports were added in JSR 201.
--
ImportDeclaration ::= StaticImportOnDemandDeclaration
\:$NoAction:\
/.$shared_NoAction./

--
-- Note that semantically, Name must be qualified to be valid (since simple
-- type names are not in scope). However, the grammar accepts simple names.
-- The use of Marker allows us to share code.
--
--SingleTypeImportDeclaration ::= 'import' TypeName ';'
SingleTypeImportDeclaration ::= 'import' Marker Name Marker Marker ';'
\:$MakeImportDeclaration:\
/.$location
void Parser::MakeImportDeclaration()
{
    AstImportDeclaration* p = ast_pool -> NewImportDeclaration();
    p -> import_token = Token(1);
    if (Token(3) > Token(2))
        p -> static_token_opt = Token(2);
    p -> name = DYNAMIC_CAST<AstName*> (Sym(3));
    if (Token(6) > Token(5))
        p -> star_token_opt = Token(5);
    p -> semicolon_token = Token(6);
    Sym(1) = p;
}
./

--
-- The use of Marker allows us to share code.
--
--TypeImportOnDemandDeclaration ::= 'import' PackageOrTypeName '.' '*' ';'
TypeImportOnDemandDeclaration ::= 'import' Marker Name '.' '*' ';'
\:$MakeImportDeclaration:\
/.$shared_function
//
// void MakeImportDeclaration();
//./

--
-- Static imports were added in JSR 201.
-- The use of Marker allows us to share code.
--
--SingleStaticImportDeclaration ::= 'import' 'static' TypeName '.'
--                                  'Identifier' ';'
SingleStaticImportDeclaration ::= 'import' 'static' Name Marker Marker ';'
\:$MakeImportDeclaration:\
/.$shared_function
//
// void MakeImportDeclaration();
//./

--
-- Static imports were added in JSR 201.
--
--StaticImportOnDemandDeclaration ::= 'import' 'static' TypeName '.' '*' ';'
StaticImportOnDemandDeclaration ::= 'import' 'static' Name '.' '*' ';'
\:$MakeImportDeclaration:\
/.$shared_function
//
// void MakeImportDeclaration();
//./

TypeDeclaration ::= ClassDeclaration
\:$NoAction:\
/.$shared_NoAction./

--
-- Enums were added in JSR 201.
--
TypeDeclaration ::= EnumDeclaration
\:$NoAction:\
/.$shared_NoAction./

TypeDeclaration ::= InterfaceDeclaration
\:$NoAction:\
/.$shared_NoAction./

--
-- Annotations were added in JSR 175.
--
TypeDeclaration ::= AnnotationTypeDeclaration
\:$NoAction:\
/.$shared_NoAction./

TypeDeclaration ::= ';'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    Sym(1) = ast_pool -> NewEmptyDeclaration(Token(1));
}
./

--18.7 Only in the LALR(1) Grammar
-- Remember that we do semantic filtering on modifiers, for every context
-- they can appear in. For better error messages, we also accept all modifiers
-- for initializer blocks, formal parameters, and local variable declarations.
--
--ClassModifiers ::= Modifiers
--FieldModifiers ::= Modifiers
--MethodModifiers ::= Modifiers
--ConstructorModifiers ::= Modifiers
--InterfaceModifiers ::= Modifiers
--ConstantModifiers ::= Modifiers
--AbstractMethodModifiers ::= Modifiers
Modifiers ::= Modifier
\:$StartList:\
/.$shared_StartList./

Modifiers ::= Modifiers Modifier
\:$AddList2:\
/.$shared_AddList2./

Modifiersopt ::= $empty
\:$NullAction:\
/.$shared_NullAction./

Modifiersopt ::= Modifiers
\:$NoAction:\
/.$shared_NoAction./

Modifier ::= 'public'
\:$MakeModifier:\
/.$location
void Parser::MakeModifier()
{
    Sym(1) = ast_pool -> NewModifierKeyword(Token(1));
}
./

Modifier ::= 'protected'
\:$MakeModifier:\
/.$shared_function
//
// void MakeModifier();
//./

Modifier ::= 'private'
\:$MakeModifier:\
/.$shared_function
//
// void MakeModifier();
//./

Modifier ::= 'static'
\:$MakeModifier:\
/.$shared_function
//
// void MakeModifier();
//./

Modifier ::= 'abstract'
\:$MakeModifier:\
/.$shared_function
//
// void MakeModifier();
//./

Modifier ::= 'final'
\:$MakeModifier:\
/.$shared_function
//
// void MakeModifier();
//./

Modifier ::= 'native'
\:$MakeModifier:\
/.$shared_function
//
// void MakeModifier();
//./

Modifier ::= 'strictfp'
\:$MakeModifier:\
/.$shared_function
//
// void MakeModifier();
//./

Modifier ::= 'synchronized'
\:$MakeModifier:\
/.$shared_function
//
// void MakeModifier();
//./

Modifier ::= 'transient'
\:$MakeModifier:\
/.$shared_function
//
// void MakeModifier();
//./

Modifier ::= 'volatile'
\:$MakeModifier:\
/.$shared_function
//
// void MakeModifier();
//./

--
-- Annotations were added in JSR 175. They are valid anywhere a modifier is,
-- additionally they are valid on package declarations.
--
Modifier ::= Annotation
\:$NoAction:\
/.$shared_NoAction./

--
-- Annotations were added in JSR 175.
--
Annotation ::= NormalAnnotation
\:$NoAction:\
/.$shared_NoAction./

--
-- Annotations were added in JSR 175.
--
Annotation ::= MarkerAnnotation
\:$NoAction:\
/.$shared_NoAction./

--
-- Annotations were added in JSR 175.
--
Annotation ::= SingleMemberAnnotation
\:$NoAction:\
/.$shared_NoAction./

--
-- Annotations were added in JSR 175.
--
--NormalAnnotation ::= '@' TypeName '(' MemberValuePairsopt ')'
NormalAnnotation ::= '@' Name '(' MemberValuePairsopt ')'
\:$MakeAnnotation:\
/.$location
void Parser::MakeAnnotation()
{
    AstAnnotation* p = ast_pool -> NewAnnotation();
    p -> at_token = Token(1);
    p -> name = DYNAMIC_CAST<AstName*> (Sym(2));
    if (Sym(4))
    {
        AstListNode* tail = DYNAMIC_CAST<AstListNode*> (Sym(4));
        p -> AllocateMemberValuePairs(tail -> index + 1);
        AstListNode* root = tail;
        do
        {
            root = root -> next;
            p -> AddMemberValuePair(DYNAMIC_CAST<AstMemberValuePair*>
                                    (root -> element));
        } while (root != tail);
        FreeCircularList(tail);
    }
    if (Token(5) > Token(3))
        p -> right_paren_token_opt = Token(5);
    Sym(1) = p;
}
./

--
-- Annotations were added in JSR 175.
--
MemberValuePairs ::= MemberValuePair
\:$StartList:\
/.$shared_StartList./

--
-- Annotations were added in JSR 175.
--
MemberValuePairs ::= MemberValuePairs ',' MemberValuePair
\:$AddList3:\
/.$shared_AddList3./

--
-- Annotations were added in JSR 175.
--
MemberValuePairsopt ::= $empty
\:$NullAction:\
/.$shared_NullAction./

--
-- Annotations were added in JSR 175.
--
MemberValuePairsopt ::= MemberValuePairs
\:$NoAction:\
/.$shared_NoAction./

--
-- Annotations were added in JSR 175. We got rid of SimpleName.
--
--MemberValuePair ::= SimpleName '=' MemberValue
MemberValuePair ::= 'Identifier' '=' MemberValue
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstMemberValuePair* p = ast_pool -> NewMemberValuePair();
    p -> identifier_token_opt = Token(1);
    p -> member_value = DYNAMIC_CAST<AstMemberValue*> (Sym(3));
    Sym(1) = p;
}
./

--
-- Annotations were added in JSR 175.
--
MemberValue ::= ConditionalExpression
\:$NoAction:\
/.$shared_NoAction./

--
-- Annotations were added in JSR 175.
--
MemberValue ::= Annotation
\:$NoAction:\
/.$shared_NoAction./

--
-- Annotations were added in JSR 175.
--
MemberValue ::= MemberValueArrayInitializer
\:$NoAction:\
/.$shared_NoAction./

--
-- Annotations were added in JSR 175. The rule was expanded inline below to
-- make the grammar LALR(1). The use of Marker allows us to share code.
--
--MemberValueArrayInitializer ::= '{' MemberValuesopt ,opt '}'
MemberValueArrayInitializer ::= '{' Marker ,opt '}'
\:$MakeArrayInitializer:\
/.$location
void Parser::MakeArrayInitializer()
{
    AstArrayInitializer* p = ast_pool -> NewArrayInitializer();
    p -> left_brace_token = Token(1);
    if (Sym(2))
    {
        AstListNode* tail = DYNAMIC_CAST<AstListNode*> (Sym(2));
        p -> AllocateVariableInitializers(tail -> index + 1);
        AstListNode* root = tail;
        do
        {
            root = root -> next;
            p -> AddVariableInitializer(DYNAMIC_CAST<AstMemberValue*>
                                        (root -> element));
        } while (root != tail);
        FreeCircularList(tail);
    }
    p -> right_brace_token = Token(4);
    Sym(1) = p;
}
./

--
-- Annotations were added in JSR 175.
--
MemberValueArrayInitializer ::= '{' MemberValues ,opt '}'
\:$MakeArrayInitializer:\
/.$shared_function
//
// void MakeArrayInitializer();
//./

--
-- Annotations were added in JSR 175.
--
MemberValues ::= MemberValue
\:$StartList:\
/.$shared_StartList./

--
-- Annotations were added in JSR 175.
--
MemberValues ::= MemberValues ',' MemberValue
\:$AddList3:\
/.$shared_AddList3./

--
-- Annotations were added in JSR 175.
-- The use of Marker allows us to share code.
--
--MarkerAnnotation ::= '@' TypeName
MarkerAnnotation ::= '@' Name Marker Marker Marker
\:$MakeAnnotation:\
/.$shared_function
//
// void MakeAnnotation();
//./

--
-- Annotations were added in JSR 175.
--
--SingleMemberAnnotation ::= '@' TypeName '(' MemberValue ')'
SingleMemberAnnotation ::= '@' Name '(' MemberValue ')'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstMemberValuePair* mvp = ast_pool -> NewMemberValuePair();
    mvp -> member_value = DYNAMIC_CAST<AstMemberValue*> (Sym(4));
    AstListNode* p = AllocateListNode();
    p -> next = p;
    p -> element = mvp;
    p -> index = 0;
    Sym(4) = p;
    MakeAnnotation();
}
./

--18.8 Productions from 8: Class Declarations
--18.8.1 Productions from 8.1: Class Declarations

--
-- Parameterized types were added in JSR 14.  We must inline expand
-- Modifiersopt to avoid ambiguity. The use of Marker allows us to share code.
--
--ClassDeclaration ::= ClassModifiersopt 'class' 'Identifier' Superopt
--                     Interfacesopt ClassBody
ClassDeclaration ::= Marker 'class' 'Identifier' TypeParametersopt
                     Superopt Interfacesopt ClassBody
\:$MakeClassDeclaration:\
/.$location
void Parser::MakeClassDeclaration()
{
    AstClassDeclaration* p = ast_pool -> NewClassDeclaration();
    p -> modifiers_opt = MakeModifiers();
    p -> class_token = Token(2);
    p -> type_parameters_opt = MakeTypeParameters(4);
    p -> super_opt = DYNAMIC_CAST<AstTypeName*> (Sym(5));
    if (Sym(6))
    {
        AstListNode* tail = DYNAMIC_CAST<AstListNode*> (Sym(6));
        p -> AllocateInterfaces(tail -> index + 1);
        AstListNode* root = tail;
        do
        {
            root = root -> next;
            p -> AddInterface(DYNAMIC_CAST<AstTypeName*> (root -> element));
        } while (root != tail);
        FreeCircularList(tail);
    }
    p -> class_body = DYNAMIC_CAST<AstClassBody*> (Sym(7));
    p -> class_body -> identifier_token = Token(3);
    p -> class_body -> owner = p;
    Sym(1) = p;
}

//
// Creates an AstTypeParameters node for insertion into an AstTypeName.
//
AstTypeParameters* Parser::MakeTypeParameters(int tokennum)
{
    if (! Sym(tokennum))
        return NULL;
    AstTypeParameters* p = ast_pool -> NewTypeParameters();
    AstListNode* tail = DYNAMIC_CAST<AstListNode*> (Sym(tokennum));
    p -> AllocateTypeParameters(tail -> index + 1);
    AstListNode* root = tail;
    do
    {
        root = root -> next;
        p -> AddTypeParameter(DYNAMIC_CAST<AstTypeParameter*>
                              (root -> element));
    } while (root != tail);
    FreeCircularList(tail);
    p -> right_angle_token = Token(tokennum + 1) - 1;
    return p;
}
./

--
-- Parameterized types were added in JSR 14.  We must inline expand
-- Modifiersopt to avoid ambiguity.
--
ClassDeclaration ::= Modifiers 'class' 'Identifier' TypeParametersopt
                     Superopt Interfacesopt ClassBody
\:$MakeClassDeclaration:\
/.$shared_function
//
// void MakeClassDeclaration();
//./

--
-- Simplify.
--
--Super ::= 'extends' ClassType
Super ::= 'extends' ClassOrInterfaceType
\:$SetSym1ToSym2:\
/.$shared_function
//
// void SetSym1ToSym2();
//./

Superopt ::= $empty
\:$NullAction:\
/.$shared_NullAction./

Superopt ::= Super
\:$NoAction:\
/.$shared_NoAction./

--
-- Simplify.
--
--Interfaces ::= 'implements' InterfaceTypeList
Interfaces ::= 'implements' TypeList
\:$SetSym1ToSym2:\
/.$shared_function
//
// void SetSym1ToSym2();
//./

Interfacesopt ::= $empty
\:$NullAction:\
/.$shared_NullAction./

Interfacesopt ::= Interfaces
\:$NoAction:\
/.$shared_NoAction./

--InterfaceTypeList ::= InterfaceType
--ClassTypeList ::= ClassType
TypeList ::= ClassOrInterfaceType
\:$StartList:\
/.$shared_StartList./

--InterfaceTypeList ::= InterfaceTypeList ',' InterfaceType
--ClassTypeList ::= ClassTypeList ',' ClassType
TypeList ::= TypeList ',' ClassOrInterfaceType
\:$AddList3:\
/.$shared_AddList3./

ClassBody ::= '{' ClassBodyDeclarationsopt '}'
\:$MakeClassBody:\
/.$location
void Parser::MakeClassBody()
{
    AstClassBody* p = ast_pool -> NewClassBody();
    if (parse_header_only)
        p -> MarkUnparsed();

    p -> left_brace_token = Token(1);
    if (Sym(2))
    {
        unsigned num_instance_variables = 0;
        unsigned num_class_variables = 0;
        unsigned num_methods = 0;
        unsigned num_constructors = 0;
        unsigned num_static_initializers = 0;
        unsigned num_instance_initializers = 0;
        unsigned num_inner_classes = 0;
        unsigned num_inner_enums = 0;
        unsigned num_inner_interfaces = 0;
        unsigned num_inner_annotations = 0;
        unsigned num_empty_declarations = 0;

        AstListNode* tail = DYNAMIC_CAST<AstListNode*> (Sym(2));
        p -> AllocateClassBodyDeclarations(tail -> index + 1);
        AstListNode* root = tail;
        do
        {
            root = root -> next;
            AstDeclared* declaration =
                DYNAMIC_CAST<AstDeclared*> (root -> element);
            AstFieldDeclaration* field_declaration =
                declaration -> FieldDeclarationCast();
            AstInitializerDeclaration* initializer =
                declaration -> InitializerDeclarationCast();
            if (field_declaration)
            {
                if (field_declaration -> modifiers_opt &&
                    field_declaration -> modifiers_opt -> static_token_opt)
                {
                    field_declaration -> MarkStatic();
                }
                //
                // Interface fields were already marked static.
                //
                if (field_declaration -> StaticFieldCast())
                    num_class_variables++;
                else num_instance_variables++;
            }
            else if (declaration -> MethodDeclarationCast())
                num_methods++;
            else if (declaration -> ConstructorDeclarationCast())
                num_constructors++;
            else if (initializer)
            {
                if (initializer -> modifiers_opt &&
                    initializer -> modifiers_opt -> static_token_opt)
                {
                    initializer -> MarkStatic();
                    num_static_initializers++;
                }
                else num_instance_initializers++;
            }
            else if (declaration -> ClassDeclarationCast())
                num_inner_classes++;
            else if (declaration -> EnumDeclarationCast())
                num_inner_enums++;
            else if (declaration -> InterfaceDeclarationCast())
                num_inner_interfaces++;
            else if (declaration -> AnnotationDeclarationCast())
                num_inner_annotations++;
            else num_empty_declarations++;
        } while (root != tail);

        p -> AllocateInstanceVariables(num_instance_variables);
        p -> AllocateClassVariables(num_class_variables);
        p -> AllocateMethods(num_methods);
        p -> AllocateConstructors(num_constructors);
        p -> AllocateStaticInitializers(num_static_initializers);
        p -> AllocateInstanceInitializers(num_instance_initializers);
        p -> AllocateNestedClasses(num_inner_classes);
        p -> AllocateNestedEnums(num_inner_enums);
        p -> AllocateNestedInterfaces(num_inner_interfaces);
        p -> AllocateNestedAnnotations(num_inner_annotations);
        p -> AllocateEmptyDeclarations(num_empty_declarations);

        root = tail;
        do
        {
            root = root -> next;
            p -> AddClassBodyDeclaration(DYNAMIC_CAST<AstDeclared*>
                                         (root -> element));
        } while (root != tail);
        FreeCircularList(tail);
    }
    p -> right_brace_token = Token(3);
    // from now on, this is the storage pool to use for this type
    p -> pool = body_pool;
    Sym(1) = p;
}
./

ClassBodyopt ::= $empty
\:$NullAction:\
/.$shared_NullAction./

ClassBodyopt ::= ClassBody
\:$NoAction:\
/.$shared_NoAction./

ClassBodyDeclarations ::= ClassBodyDeclaration
\:$StartList:\
/.$shared_StartList./

ClassBodyDeclarations ::= ClassBodyDeclarations ClassBodyDeclaration
\:$AddList2:\
/.$shared_AddList2./

ClassBodyDeclarationsopt ::= $empty
\:$NullAction:\
/.$shared_NullAction./

ClassBodyDeclarationsopt ::= ClassBodyDeclarations
\:$NoAction:\
/.$shared_NoAction./

--
-- Simplify. See below.
--
--ClassBodyDeclaration ::= ClassMemberDeclaration

ClassBodyDeclaration ::= ConstructorDeclaration
\:$NoAction:\
/.$shared_NoAction./

--
-- For nicer semantic error messages, we lump static and instance initializers
-- together. Also, we parse arbitrary modifiers, but semantically only accept
-- static or no modifiers.
--
--ClassBodyDeclaration ::= StaticInitializer
--ClassBodyDeclaration ::= MethodBody
ClassBodyDeclaration ::= InitializerDeclaration
\:$NoAction:\
/.$shared_NoAction./

--
-- Simplify.
--
--ClassMemberDeclaration ::= FieldDeclaration
ClassBodyDeclaration ::= FieldDeclaration
\:$NoAction:\
/.$shared_NoAction./

--
-- Simplify.
--
--ClassMemberDeclaration ::= MethodDeclaration
ClassBodyDeclaration ::= MethodDeclaration
\:$NoAction:\
/.$shared_NoAction./

--1.1 feature
--
-- Consolidate and simplify.
--ClassMemberDeclaration ::= ClassDeclaration
--ClassMemberDeclaration ::= InterfaceDeclaration
--ClassMemberDeclaration ::= ';'
--
ClassBodyDeclaration ::= TypeDeclaration
\:$NoAction:\
/.$shared_NoAction./

--18.8.2 Productions from 8.3: Field Declarations

--
-- The use of Marker allows us to share code.  We must inline expand
-- Modifiersopt to avoid ambiguity.
--
--FieldDeclaration ::= FieldModifiersopt Type VariableDeclarators ';'
FieldDeclaration ::= Marker Marker Type VariableDeclarators ';'
\:$MakeFieldDeclaration:\
/.$location
void Parser::MakeFieldDeclaration()
{
    AstFieldDeclaration* p = ast_pool -> NewFieldDeclaration();
    p -> modifiers_opt = MakeModifiers();
    p -> type = DYNAMIC_CAST<AstType*> (Sym(3));
    AstListNode* tail = DYNAMIC_CAST<AstListNode*> (Sym(4));
    p -> AllocateVariableDeclarators(tail -> index + 1);
    AstListNode* root = tail;
    do
    {
        root = root -> next;
        p -> AddVariableDeclarator(DYNAMIC_CAST<AstVariableDeclarator*>
                                   (root -> element));
    } while (root != tail);
    FreeCircularList(tail);
    p -> semicolon_token = Token(5);
    Sym(1) = p;
}
./

FieldDeclaration ::= Modifiers Marker Type VariableDeclarators ';'
\:$MakeFieldDeclaration:\
/.$shared_function
//
// void MakeFieldDeclaration();
//./

VariableDeclarators ::= VariableDeclarator
\:$StartList:\
/.$shared_StartList./

VariableDeclarators ::= VariableDeclarators ',' VariableDeclarator
\:$AddList3:\
/.$shared_AddList3./

VariableDeclarator ::= VariableDeclaratorId
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstVariableDeclarator* p = ast_pool -> NewVariableDeclarator();
    p -> variable_declarator_name =
        DYNAMIC_CAST<AstVariableDeclaratorId*> (Sym(1));
    Sym(1) = p;
}
./

VariableDeclarator ::= VariableDeclaratorId '=' VariableInitializer
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstVariableDeclarator* p = ast_pool -> NewVariableDeclarator();
    p -> variable_declarator_name =
        DYNAMIC_CAST<AstVariableDeclaratorId*> (Sym(1));
    p -> variable_initializer_opt = Sym(3);
    Sym(1) = p;
}
./

VariableDeclaratorId ::= 'Identifier' Dimsopt
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstVariableDeclaratorId* p = ast_pool -> NewVariableDeclaratorId();
    p -> identifier_token = Token(1);
    p -> brackets_opt = DYNAMIC_CAST<AstBrackets*> (Sym(2));
    Sym(1) = p;
}
./

VariableInitializer ::= Expression
\:$NoAction:\
/.$shared_NoAction./

VariableInitializer ::= ArrayInitializer
\:$NoAction:\
/.$shared_NoAction./

--18.8.3 Productions from 8.4: Method Declarations
--
-- We use "MethodHeaderMarker" to speed up parsing while minimizing memory.
-- During the first pass, we only care about declarations, so we skip
-- everything inside { }. On the second pass, we parse only one method at a
-- time (see the production of Goal above).
--
-- Also, we expanded MethodBody inline to enable the sharing of MethodBody
-- between methods, constructors, and initializers. Note that MethodBody
-- can support an explicit constructor call; so it requires semantic filtering.
--
--MethodDeclaration ::= MethodHeader MethodBody
MethodDeclaration ::= MethodHeader MethodHeaderMarker MethodBody
\:$MakeMethodDeclaration:\
/.$location
void Parser::MakeMethodDeclaration()
{
    AstMethodDeclaration* p = DYNAMIC_CAST<AstMethodDeclaration*> (Sym(1));
    if (Sym(3))
        p -> method_body_opt = DYNAMIC_CAST<AstMethodBody*> (Sym(3));
    else p -> semicolon_token_opt = Token(4);
}
./

--
-- The use of Marker allows us to share code.
--
MethodDeclaration ::= MethodHeader MethodHeaderMarker Marker ';'
\:$MakeMethodDeclaration:\
/.$shared_function
//
// void MakeMethodDeclaration();
//./

--
-- The use of Marker allows us to share code.  We must inline expand
-- Modifiersopt to avoid ambiguity.
--
--MethodHeader ::= MethodModifiersopt Type MethodDeclarator Throwsopt
MethodHeader ::= Marker Marker Type MethodDeclarator Throwsopt
\:$MakeMethodHeader:\
/.$location
void Parser::MakeMethodHeader()
{
    AstMethodDeclaration* p = ast_pool -> NewMethodDeclaration();
    p -> modifiers_opt = MakeModifiers();
    p -> type_parameters_opt = MakeTypeParameters(2);
    p -> type = DYNAMIC_CAST<AstType*> (Sym(3));
    p -> method_declarator = DYNAMIC_CAST<AstMethodDeclarator*> (Sym(4));
    if (Sym(5))
    {
        AstListNode* tail = DYNAMIC_CAST<AstListNode*> (Sym(5));
        p -> AllocateThrows(tail -> index + 1);
        AstListNode* root = tail;
        do
        {
            root = root -> next;
            p -> AddThrow(DYNAMIC_CAST<AstTypeName*> (root -> element));
        } while (root != tail);
        FreeCircularList(tail);
    }
    Sym(1) = p;
}
./

--
-- The use of Marker allows us to share code.  We must inline expand
-- Modifiersopt to avoid ambiguity.
--
MethodHeader ::= Modifiers Marker Type MethodDeclarator Throwsopt
\:$MakeMethodHeader:\
/.$shared_function
//
// void MakeMethodHeader();
//./

--
-- Parameterized types were added in JSR 14.  We must inline expand
-- Modifiersopt to avoid ambiguity. The use of Marker allows us to share code.
--
--MethodHeader ::= MethodModifiersopt Type MethodDeclarator Throwsopt
MethodHeader ::= Marker TypeParameters Type MethodDeclarator Throwsopt
\:$MakeMethodHeader:\
/.$shared_function
//
// void MakeMethodHeader();
//./

--
-- Parameterized types were added in JSR 14.  We must inline expand
-- Modifiersopt to avoid ambiguity.
--
MethodHeader ::= Modifiers TypeParameters Type MethodDeclarator Throwsopt
\:$MakeMethodHeader:\
/.$shared_function
//
// void MakeMethodHeader();
//./

--
-- The use of Marker allows us to share code.  We must inline expand
-- Modifiersopt to avoid ambiguity.
--
--MethodHeader ::= MethodModifiersopt 'void' MethodDeclarator Throwsopt
MethodHeader ::= Marker Marker VoidType MethodDeclarator Throwsopt
\:$MakeMethodHeader:\
/.$shared_function
//
// void MakeMethodHeader();
//./

--
-- The use of Marker allows us to share code.  We must inline expand
-- Modifiersopt to avoid ambiguity.
--
MethodHeader ::= Modifiers Marker VoidType MethodDeclarator Throwsopt
\:$MakeMethodHeader:\
/.$shared_function
//
// void MakeMethodHeader();
//./

--
-- Parameterized types were added in JSR 14.  We must inline expand
-- Modifiersopt to avoid ambiguity. The use of Marker allows us to share code.
--
--MethodHeader ::= Modifiersopt 'void' MethodDeclarator Throwsopt
MethodHeader ::= Marker TypeParameters VoidType MethodDeclarator Throwsopt
\:$MakeMethodHeader:\
/.$shared_function
//
// void MakeMethodHeader();
//./

--
-- Parameterized types were added in JSR 14.  We must inline expand
-- Modifiersopt to avoid ambiguity.
--
MethodHeader ::= Modifiers TypeParameters VoidType MethodDeclarator Throwsopt
\:$MakeMethodHeader:\
/.$shared_function
//
// void MakeMethodHeader();
//./

MethodDeclarator ::= 'Identifier' '(' FormalParameterListopt ')' Dimsopt
\:$MakeMethodDeclarator:\
/.$location
void Parser::MakeMethodDeclarator()
{
    AstMethodDeclarator* p = ast_pool -> NewMethodDeclarator();
    p -> identifier_token = Token(1);
    p -> left_parenthesis_token = Token(2);
    if (Sym(3))
    {
        AstListNode* tail = DYNAMIC_CAST<AstListNode*> (Sym(3));
        p -> AllocateFormalParameters(tail -> index + 1);
        AstListNode* root = tail;
        do
        {
            root = root -> next;
            p -> AddFormalParameter(DYNAMIC_CAST<AstFormalParameter*>
                                    (root -> element));
        } while (root != tail);
        FreeCircularList(tail);
    }
    p -> right_parenthesis_token = Token(4);
    p -> brackets_opt = DYNAMIC_CAST<AstBrackets*> (Sym(5));
    Sym(1) = p;
}
./

--
-- Varargs were added in JSR 201.
--
--FormalParameterList ::= FormalParameter
FormalParameterList ::= LastFormalParameter
\:$StartList:\
/.$shared_StartList./

--
-- Varargs were added in JSR 201.
--
--FormalParameterList ::= FormalParameterList ',' FormalParameter
FormalParameterList ::= FormalParameters ',' LastFormalParameter
\:$AddList3:\
/.$shared_AddList3./

FormalParameterListopt ::= $empty
\:$NullAction:\
/.$shared_NullAction./

FormalParameterListopt ::= FormalParameterList
\:$NoAction:\
/.$shared_NoAction./

--
-- Varargs were added in JSR 201.
--
FormalParameters ::= FormalParameter
\:$StartList:\
/.$shared_StartList./

--
-- Varargs were added in JSR 201.
--
FormalParameters ::= FormalParameters ',' FormalParameter
\:$AddList3:\
/.$shared_AddList3./

--
-- For nicer error messages, we accept all modifiers, even though only
-- 'final' and annotations are valid. Also, we must inline expand finalopt
-- to avoid ambiguity. The use of Marker allows us to share code.
--
--FormalParameter ::= finalopt Type VariableDeclaratorId
FormalParameter ::= Type Marker Marker VariableDeclaratorId
\:$MakeFormalParameter:\
/.$location
void Parser::MakeFormalParameter()
{
    AstFormalParameter* p = ast_pool -> NewFormalParameter();
    if (Sym(2))
    {
        p -> modifiers_opt = MakeModifiers();
        p -> type = DYNAMIC_CAST<AstType*> (Sym(2));
    }
    else p -> type = DYNAMIC_CAST<AstType*> (Sym(1));
    if (Token(4) > Token(3))
        p -> ellipsis_token_opt = Token(3);
    AstVariableDeclarator* formal_declarator =
        ast_pool -> NewVariableDeclarator();
    formal_declarator -> variable_declarator_name =
        DYNAMIC_CAST<AstVariableDeclaratorId*> (Sym(4));
    p -> formal_declarator = formal_declarator;
    Sym(1) = p;
}
./

--1.1 feature
--
-- For nicer error messages, we accept all modifiers, even though only
-- 'final' and annotations are valid. Also, we must inline expand finalopt
-- to avoid ambiguity. The use of Marker allows us to share code.
--
--FormalParameter ::= final Type VariableDeclaratorId
FormalParameter ::= Modifiers Type Marker VariableDeclaratorId
\:$MakeFormalParameter:\
/.$shared_function
//
// void MakeFormalParameter();
//./

--
-- Varargs were added in JSR 201. We must match the inline expansion of
-- FormalParameter to avoid ambiguity.
--
--LastFormalParameter ::= Modifiersopt Type ...opt VariableDeclaratorId
LastFormalParameter ::= FormalParameter
\:$NoAction:\
/.$shared_NoAction./

--
-- Varargs were added in JSR 201. The use of Marker allows us to share code.
--
LastFormalParameter ::= Type Marker '...' VariableDeclaratorId
\:$MakeFormalParameter:\
/.$shared_function
//
// void MakeFormalParameter();
//./

--
-- Varargs were added in JSR 201.
-- For nicer error messages, we accept all modifiers, even though only
-- 'final' and annotations are valid.
--
LastFormalParameter ::= Modifiers Type '...' VariableDeclaratorId
\:$MakeFormalParameter:\
/.$shared_function
//
// void MakeFormalParameter();
//./

--
-- Simplify.
--
--Throws ::= 'throws' ClassTypeList
Throws ::= 'throws' TypeList
\:$SetSym1ToSym2:\
/.$shared_function
//
// void SetSym1ToSym2();
//./

Throwsopt ::= $empty
\:$NullAction:\
/.$shared_NullAction./

Throwsopt ::= Throws
\:$NoAction:\
/.$shared_NoAction./

--
-- Notice that we filter out an initial explicit constructor invocation,
-- since we have modified Statement to include this() and super(). Other
-- explicit constructor calls are ignored now, and flagged as errors
-- during semantic analysis.
--
MethodBody ::= '{' BlockStatementsopt '}'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstMethodBody* p = ast_pool -> NewMethodBody();
    p -> left_brace_token = Token(1);
    p -> right_brace_token = Token(3);

    if (Sym(2))
    {
        AstListNode* tail = DYNAMIC_CAST<AstListNode*> (Sym(2));
        // Allocate 1 extra for possible generated return statement.
        p -> AllocateStatements(tail -> index + 2);
        AstListNode* root = tail -> next;
        if (root -> element -> IsExplicitConstructorInvocation())
            p -> explicit_constructor_opt =
                DYNAMIC_CAST<AstStatement*> (root -> element);
        else
            p -> AddStatement(DYNAMIC_CAST<AstStatement*> (root -> element));
        while (root != tail)
        {
            root = root -> next;
            p -> AddStatement(DYNAMIC_CAST<AstStatement*> (root -> element));
        }
        FreeCircularList(tail);
    }
    else p -> AllocateStatements(1);
    Sym(1) = p;
}
./

--
-- Instead of directly including this rule, we have expanded it inline above.
--
--MethodBody ::= ';'
--

--18.8.4 Productions from 8.5: Static Initializers
--
-- For nicer error messages, we accept arbitrary modifiers. Thus this rule can
-- parse static and instance initializers. The use of MethodHeaderMarker
-- allows the 2-pass parsing. See comments of MethodDeclaration.
-- The use of Marker allows us to share code.
--
--StaticInitializer ::= 'static' MethodBody
InitializerDeclaration ::= Marker MethodHeaderMarker MethodBody
\:$MakeInitializerDeclaration:\
/.$location
void Parser::MakeInitializerDeclaration()
{
    AstInitializerDeclaration* p = ast_pool -> NewInitializerDeclaration();
    p -> modifiers_opt = MakeModifiers();
    p -> block = DYNAMIC_CAST<AstMethodBody*> (Sym(3));
    Sym(1) = p;
}
./

-- For nicer error messages, we accept arbitrary modifiers. Thus this rule can
-- parse static and instance initializers. The use of MethodHeaderMarker
-- allows the 2-pass parsing. See comments of MethodDeclaration.
--
InitializerDeclaration ::= Modifiers MethodHeaderMarker MethodBody
\:$MakeInitializerDeclaration:\
/.$shared_function
//
// void MakeInitializerDeclaration();
//./

--18.8.5 Productions from 8.6: Constructor Declarations
--
-- The use of Marker allows us to share code. MethodHeaderMarker allows us to
-- do 2-pass parsing, and MethodBody was rewritten to handle constructor
-- bodies. We must inline expand Modifiersopt to avoid ambiguity.
--
--ConstructorDeclaration ::= ConstructorModifiersopt ConstructorDeclarator
--                           Throwsopt ConstructorBody
ConstructorDeclaration ::= Marker Marker ConstructorDeclarator Throwsopt
                           MethodHeaderMarker MethodBody
\:$MakeConstructorDeclaration:\
/.$location
void Parser::MakeConstructorDeclaration()
{
    AstConstructorDeclaration* p = ast_pool -> NewConstructorDeclaration();
    p -> modifiers_opt = MakeModifiers();
    p -> type_parameters_opt = MakeTypeParameters(2);
    p -> constructor_declarator = DYNAMIC_CAST<AstMethodDeclarator*> (Sym(3));
    if (Sym(4))
    {
        AstListNode* tail = DYNAMIC_CAST<AstListNode*> (Sym(4));
        p -> AllocateThrows(tail -> index + 1);
        AstListNode* root = tail;
        do
        {
            root = root -> next;
            p -> AddThrow(DYNAMIC_CAST<AstTypeName*> (root -> element));
        } while (root != tail);
        FreeCircularList(tail);
    }
    p -> constructor_body = DYNAMIC_CAST<AstMethodBody*> (Sym(6));
    Sym(1) = p;
}
./

--
-- The use of Marker allows us to share code. MethodHeaderMarker allows us to
-- do 2-pass parsing, and MethodBody was rewritten to handle constructor
-- bodies. We must inline expand Modifiersopt to avoid ambiguity.
--
ConstructorDeclaration ::= Modifiers Marker ConstructorDeclarator Throwsopt
                           MethodHeaderMarker MethodBody
\:$MakeConstructorDeclaration:\
/.$shared_function
//
// void MakeConstructorDeclaration();
//./

--
-- Parameterized types were added in JSR 14.  We must inline expand
-- Modifiersopt to avoid ambiguity. The use of Marker allows us to share code.
--
--ConstructorDeclaration ::= ConstructorModifiersopt ConstructorDeclarator
--                           Throwsopt ConstructorBody
ConstructorDeclaration ::= Marker TypeParameters ConstructorDeclarator
                           Throwsopt MethodHeaderMarker MethodBody
\:$MakeConstructorDeclaration:\
/.$shared_function
//
// void MakeConstructorDeclaration();
//./

--
-- Parameterized types were added in JSR 14.  We must inline expand
-- Modifiersopt to avoid ambiguity.
--
ConstructorDeclaration ::= Modifiers TypeParameters ConstructorDeclarator
                           Throwsopt MethodHeaderMarker MethodBody
\:$MakeConstructorDeclaration:\
/.$shared_function
//
// void MakeConstructorDeclaration();
//./

--
-- The use of Marker allows us to share code. Also, we got rid of SimpleName.
--
--ConstructorDeclarator ::= SimpleName '(' FormalParameterListopt ')'
ConstructorDeclarator ::= 'Identifier' '(' FormalParameterListopt ')' Marker
\:$MakeMethodDeclarator:\
/.$shared_function
//
// void MakeMethodDeclarator();
//./

--
-- For better error reporting, we have coalesced ExplicitConstructorInvocation
-- into BlockStatement. Therefore, we do not need a rule for ConstructorBody,
-- since MethodBody does the same amount of work. During semantic analysis,
-- we then check calls of an explicit constructor invocation out of context.
--
--ConstructorBody ::= '{' ExplicitConstructorInvocationopt
--                    BlockStatementsopt '}'
--

--
-- Simplify.
--
--ExplicitConstructorInvocation ::= 'this' '(' ArgumentListopt ')' ';'
ExplicitConstructorInvocation ::= 'this' Arguments ';'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstThisCall* p = ast_pool -> NewThisCall();
    p -> this_token = Token(1);
    p -> arguments = DYNAMIC_CAST<AstArguments*> (Sym(2));
    p -> semicolon_token = Token(3);
    Sym(1) = p;
}
./

--
-- Parameterized types were added in JSR 14.
--
ExplicitConstructorInvocation ::= TypeArguments 'this' Arguments ';'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstThisCall* p = ast_pool -> NewThisCall();
    p -> type_arguments_opt = MakeExplicitTypeArguments(1);
    p -> this_token = Token(2);
    p -> arguments = DYNAMIC_CAST<AstArguments*> (Sym(3));
    p -> semicolon_token = Token(4);
    Sym(1) = p;
}

//
// Given TypeArgumentsopt at tokennum, generate the AstTypeArguments for use
// in a method call's explicit type arguments. There must be a production
// at tokennum + 1 to allow finding the closing '>'.
//
AstTypeArguments* Parser::MakeExplicitTypeArguments(int tokennum)
{
    if (! Sym(tokennum))
        return NULL;
    AstTypeArguments* p =
        ast_pool -> NewTypeArguments(Token(tokennum),
                                     Token(tokennum + 1) - 1);
    AstListNode* tail = DYNAMIC_CAST<AstListNode*> (Sym(tokennum));
    p -> AllocateTypeArguments(tail -> index + 1);
    AstListNode* root = tail;
    do
    {
        root = root -> next;
        p -> AddTypeArgument(DYNAMIC_CAST<AstType*> (root -> element));
    } while (root != tail);
    FreeCircularList(tail);
    return p;
}
./

--
-- Simplify.
--
--ExplicitConstructorInvocation ::= 'super' '(' ArgumentListopt ')' ';'
ExplicitConstructorInvocation ::= 'super' Arguments ';'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstSuperCall* p = ast_pool -> NewSuperCall();
    p -> super_token = Token(1);
    p -> arguments = DYNAMIC_CAST<AstArguments*> (Sym(2));
    p -> semicolon_token = Token(3);
    Sym(1) = p;
}
./

--
-- Parameterized types were added in JSR 14.
--
ExplicitConstructorInvocation ::= TypeArguments 'super' Arguments ';'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstSuperCall* p = ast_pool -> NewSuperCall();
    p -> type_arguments_opt = MakeExplicitTypeArguments(1);
    p -> super_token = Token(2);
    p -> arguments = DYNAMIC_CAST<AstArguments*> (Sym(3));
    p -> semicolon_token = Token(4);
    Sym(1) = p;
}
./

--1.1 feature
--
-- Parameterized types were added in JSR 14.
--
--ExplicitConstructorInvocation ::= Primary '.' 'super' '(' ArgumentListopt ')'
--                                  ';'
ExplicitConstructorInvocation ::= Primary '.' TypeArgumentsopt 'super'
                                  Arguments ';'
\:$MakeQualifiedSuper:\
/.$location
void Parser::MakeQualifiedSuper()
{
    AstSuperCall* p = ast_pool -> NewSuperCall();
    p -> base_opt = DYNAMIC_CAST<AstExpression*> (Sym(1));
    p -> type_arguments_opt = MakeExplicitTypeArguments(3);
    p -> super_token = Token(4);
    p -> arguments = DYNAMIC_CAST<AstArguments*> (Sym(5));
    p -> semicolon_token = Token(6);
    Sym(1) = p;
}
./

--1.1 feature
--
-- The use of Marker allows us to share code.
--
--ExplicitConstructorInvocation ::= Name '.' 'super' '(' ArgumentListopt ')'
--                                  ';'
ExplicitConstructorInvocation ::= Name '.' Marker 'super' Arguments ';'
\:$MakeQualifiedSuper:\
/.$shared_function
//
// void MakeQualifiedSuper();
//./

--1.1 feature
--
-- Parameterized types were added in JSR 14.
--
--ExplicitConstructorInvocation ::= Name '.' 'super' '(' ArgumentListopt ')'
--                                  ';'
ExplicitConstructorInvocation ::= Name '.' TypeArguments 'super' Arguments ';'
\:$MakeQualifiedSuper:\
/.$shared_function
//
// void MakeQualifiedSuper();
//./

--
-- Enums were added in JSR 201. We must inline expand Modifiersopt to avoid
-- ambiguity. The use of Marker allows us to share code.
--
--EnumDeclaration ::= ClassModifiersopt 'enum' 'Identifier' Interfacesopt
--                    EnumBody
EnumDeclaration ::= Marker 'enum' 'Identifier' Interfacesopt EnumBody
\:$MakeEnumDeclaration:\
/.$location
void Parser::MakeEnumDeclaration()
{
    //
    // Because of how we handle EnumConstants, the EnumBody production already
    // created the AstEnumDeclaration, and populated the class_body field.
    //
    AstEnumDeclaration* p = DYNAMIC_CAST<AstEnumDeclaration*> (Sym(5));
    p -> modifiers_opt = MakeModifiers();
    p -> enum_token = Token(2);
    p -> class_body -> identifier_token = Token(3);
    if (Sym(4))
    {
        AstListNode* tail = DYNAMIC_CAST<AstListNode*> (Sym(4));
        p -> AllocateInterfaces(tail -> index + 1);
        AstListNode* root = tail;
        do
        {
            root = root -> next;
            p -> AddInterface(DYNAMIC_CAST<AstTypeName*> (root -> element));
        } while (root != tail);
        FreeCircularList(tail);
    }
    p -> class_body -> identifier_token = Token(3);
    Sym(1) = p;
}
./

--
-- Enums were added in JSR 201. We must inline expand Modifiersopt to avoid
-- ambiguity. The use of Marker allows us to share code.
--
EnumDeclaration ::= Modifiers 'enum' 'Identifier' Interfacesopt EnumBody
\:$MakeEnumDeclaration:\
/.$shared_function
//
// void MakeEnumDeclaration();
//./

--
-- Enums were added in JSR 201. The rule was expanded inline below to
-- make the grammar LALR(1). The use of Marker allows us to share code.
--
--EnumBody ::= '{' EnumConstantsopt ,opt EnumBodyDeclarationsopt '}'
EnumBody ::= '{' Marker ,opt EnumBodyDeclarationsopt '}'
\:$MakeEnumBody:\
/.$location
void Parser::MakeEnumBody()
{
    //
    // The class_body was either created in EnumBodyDeclarations, or we must
    // generate it here.
    //
    AstEnumDeclaration* p = ast_pool -> NewEnumDeclaration();
    if (Sym(2))
    {
        AstListNode* tail = DYNAMIC_CAST<AstListNode*> (Sym(2));
        p -> AllocateEnumConstants(tail -> index + 1);
        AstListNode* root = tail;
        do
        {
            root = root -> next;
            p -> AddEnumConstant(DYNAMIC_CAST<AstEnumConstant*>
                                 (root -> element));
        } while (root != tail);
        FreeCircularList(tail);
    }
    if (Sym(4))
    {
        p -> class_body = DYNAMIC_CAST<AstClassBody*> (Sym(4));
    }
    else
    {
        p -> class_body = ast_pool -> NewClassBody();
        p -> class_body -> right_brace_token = Token(5);
        p -> class_body -> pool = body_pool;
    }
    p -> class_body -> left_brace_token = Token(1);
    p -> class_body -> owner = p;
    Sym(1) = p;
}
./

--
-- Enums were added in JSR 201.
--
EnumBody ::= '{' EnumConstants ,opt EnumBodyDeclarationsopt '}'
\:$MakeEnumBody:\
/.$shared_function
//
// void MakeEnumBody();
//./

--
-- Enums were added in JSR 201.
--
EnumConstants ::= EnumConstant
\:$StartList:\
/.$shared_StartList./

--
-- Enums were added in JSR 201.
--
EnumConstants ::= EnumConstants ',' EnumConstant
\:$AddList3:\
/.$shared_AddList3./

--
-- Enums were added in JSR 201.
--
EnumConstant ::= Modifiersopt 'Identifier' Argumentsopt ClassBodyopt
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstEnumConstant* p = ast_pool -> NewEnumConstant(Token(2));
    p -> modifiers_opt = MakeModifiers();
    p -> arguments_opt = DYNAMIC_CAST<AstArguments*> (Sym(3));
    p -> class_body_opt = DYNAMIC_CAST<AstClassBody*> (Sym(4));
    Sym(1) = p;
}
./

--
-- Enums were added in JSR 201. As long as enums introduce this production, we
-- use it elsewhere, too.
--
Arguments ::= '(' ArgumentListopt ')'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstArguments* p = ast_pool -> NewArguments(Token(1), Token(3));
    if (Sym(2))
    {
        AstListNode* tail = DYNAMIC_CAST<AstListNode*> (Sym(2));
        p -> AllocateArguments(tail -> index + 1);
        AstListNode* root = tail;
        do
        {
            root = root -> next;
            p -> AddArgument(DYNAMIC_CAST<AstExpression*> (root -> element));
        } while (root != tail);
        FreeCircularList(tail);
    }
    Sym(1) = p;
}
./

--
-- Enums were added in JSR 201.
--
Argumentsopt ::= $empty
\:$NullAction:\
/.$shared_NullAction./

--
-- Enums were added in JSR 201.
--
Argumentsopt ::= Arguments
\:$NoAction:\
/.$shared_NoAction./

--
-- Enums were added in JSR 201. The use of Marker allows us to share code.
--
--EnumBodyDeclarations ::= ';' ClassBodyDeclarationsopt
EnumBodyDeclarations ::= ';' ClassBodyDeclarationsopt Marker
\:$MakeClassBody:\
/.$shared_function
//
// void MakeClassBody();
//./

--
-- Enums were added in JSR 201.
--
EnumBodyDeclarationsopt ::= $empty
\:$NullAction:\
/.$shared_NullAction./

--
-- Enums were added in JSR 201.
--
EnumBodyDeclarationsopt ::= EnumBodyDeclarations
\:$NoAction:\
/.$shared_NoAction./

--18.9 Productions from 9: Interface Declarations
--18.9.1 Productions from 9.1: Interface Declarations
--
-- Parameterized types were added in JSR 14.  We must inline expand
-- Modifiersopt to avoid ambiguity. The use of Marker allows us to share code.
--
--InterfaceDeclaration ::= InterfaceModifiersopt 'interface' 'Identifier'
--                         ExtendsInterfacesopt InterfaceBody
InterfaceDeclaration ::= Marker 'interface' 'Identifier' TypeParametersopt
                         ExtendsInterfacesopt InterfaceBody
\:$MakeInterfaceDeclaration:\
/.$location
void Parser::MakeInterfaceDeclaration()
{
    AstInterfaceDeclaration* p = ast_pool -> NewInterfaceDeclaration();
    p -> modifiers_opt = MakeModifiers();
    p -> interface_token = Token(2);
    p -> type_parameters_opt = MakeTypeParameters(4);
    if (Sym(5))
    {
        AstListNode* tail = DYNAMIC_CAST<AstListNode*> (Sym(5));
        p -> AllocateInterfaces(tail -> index + 1);
        AstListNode* root = tail;
        do
        {
            root = root -> next;
            p -> AddInterface(DYNAMIC_CAST<AstTypeName*> (root -> element));
        } while (root != tail);
        FreeCircularList(tail);
    }
    p -> class_body = DYNAMIC_CAST<AstClassBody*> (Sym(6));
    p -> class_body -> identifier_token = Token(3);
    p -> class_body -> owner = p;
    Sym(1) = p;
}
./

--
-- Parameterized types were added in JSR 14.  We must inline expand
-- Modifiersopt to avoid ambiguity.
--
InterfaceDeclaration ::= Modifiers 'interface' 'Identifier' TypeParametersopt
                         ExtendsInterfacesopt InterfaceBody
\:$MakeInterfaceDeclaration:\
/.$shared_function
//
// void MakeInterfaceDeclaration();
//./

--
-- Simplify.
--
--ExtendsInterfaces ::= 'extends' InterfaceTypeList
ExtendsInterfaces ::= 'extends' TypeList
\:$SetSym1ToSym2:\
/.$shared_function
//
// void SetSym1ToSym2();
//./

ExtendsInterfacesopt ::= $empty
\:$NullAction:\
/.$shared_NullAction./

ExtendsInterfacesopt ::= ExtendsInterfaces
\:$NoAction:\
/.$shared_NoAction./

InterfaceBody ::= '{' InterfaceMemberDeclarationsopt '}'
\:$MakeClassBody:\
/.$shared_function
//
// void MakeClassBody();
//./

InterfaceMemberDeclarations ::= InterfaceMemberDeclaration
\:$StartList:\
/.$shared_StartList./

InterfaceMemberDeclarations ::= InterfaceMemberDeclarations
                                InterfaceMemberDeclaration
\:$AddList2:\
/.$shared_AddList2./

InterfaceMemberDeclarationsopt ::= $empty
\:$NullAction:\
/.$shared_NullAction./

InterfaceMemberDeclarationsopt ::= InterfaceMemberDeclarations
\:$NoAction:\
/.$shared_NoAction./

--
--
-- For less code duplication and better semantic messages, we parse
-- non-abstract method bodies and non-initialized fields now, then do a
-- semantic check that this was valid.
--
InterfaceMemberDeclaration ::= ConstantDeclaration
\:$NoAction:\
/.$shared_NoAction./

--
-- Simplify. See below.
--
--InterfaceMemberDeclaration ::= AbstractMethodDeclaration

--1.1 feature
--
-- Consolidate and simplify.
--
--InterfaceMemberDeclaration ::= ClassDeclaration
--InterfaceMemberDeclaration ::= InterfaceDeclaration
--InterfaceMemberDeclaration ::= ';'
InterfaceMemberDeclaration ::= TypeDeclaration
\:$NoAction:\
/.$shared_NoAction./

ConstantDeclaration ::= FieldDeclaration
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    DYNAMIC_CAST<AstFieldDeclaration*> (Sym(1)) -> MarkStatic();
}
./

--
-- Simplify.
--
--AbstractMethodDeclaration ::= MethodHeader ';'
InterfaceMemberDeclaration ::= MethodDeclaration
\:$NoAction:\
/.$shared_NoAction./

--
-- Annotations were added in JSR 175. We must inline expand Modifiersopt to
-- avoid ambiguity: does 'public @' start 'public @A class B{}' or
-- 'public @interface A{}'. The use of Marker allows us to share code.
--
--AnnotationTypeDeclaration ::= InterfaceModifiersopt '@' 'interface'
--                              'Identifier' AnnotationTypeBody
AnnotationTypeDeclaration ::= '@' Marker 'interface' 'Identifier'
                              AnnotationTypeBody
\:$MakeAnnotationTypeDeclaration:\
/.$location
void Parser::MakeAnnotationTypeDeclaration()
{
    AstAnnotationDeclaration* p =
        ast_pool -> NewAnnotationDeclaration(Token(3));
    if (Token(3) > Token(2))
        p -> modifiers_opt = MakeModifiers();
    p -> class_body = DYNAMIC_CAST<AstClassBody*> (Sym(5));
    p -> class_body -> identifier_token = Token(4);
    p -> class_body -> owner = p;
    Sym(1) = p;
}
./

-- Annotations were added in JSR 175. We must inline expand Modifiersopt to
-- avoid ambiguity: does 'public @' start 'public @A class B{}' or
-- 'public @interface A{}'.
--
AnnotationTypeDeclaration ::= Modifiers '@' 'interface' 'Identifier'
                              AnnotationTypeBody
\:$MakeAnnotationTypeDeclaration:\
/.$shared_function
//
// void MakeAnnotationTypeDeclaration();
//./

--
-- Annotations were added in JSR 175.
--
AnnotationTypeBody ::= '{' AnnotationTypeMemberDeclarationsopt '}'
\:$MakeClassBody:\
/.$shared_function
//
// void MakeClassBody();
//./

--
-- Annotations were added in JSR 175.
--
AnnotationTypeMemberDeclarations ::= AnnotationTypeMemberDeclaration
\:$StartList:\
/.$shared_StartList./

--
-- Annotations were added in JSR 175.
--
AnnotationTypeMemberDeclarations ::= AnnotationTypeMemberDeclarations
                                     AnnotationTypeMemberDeclaration
\:$AddList2:\
/.$shared_AddList2./

--
-- Annotations were added in JSR 175.
--
AnnotationTypeMemberDeclarationsopt ::= $empty
\:$NullAction:\
/.$shared_NullAction./

--
-- Annotations were added in JSR 175.
--
AnnotationTypeMemberDeclarationsopt ::= AnnotationTypeMemberDeclarations
\:$NoAction:\
/.$shared_NoAction./

--
-- Annotations were added in JSR 175. We must inline expand Modifiersopt to
-- avoid ambiguity. The use of Marker allows us to share code.
--
--AnnotationTypeMemberDeclaration ::= AbstractMethodModifiersopt Type
--                                    'Identifier' '(' ')' DefaultValueopt ';'
AnnotationTypeMemberDeclaration ::= Marker Marker Type 'Identifier' '(' ')'
                                    DefaultValueopt ';'
\:$MakeAnnotationTypeMemberDeclaration:\
/.$location
void Parser::MakeAnnotationTypeMemberDeclaration()
{
    AstMethodDeclaration* p = ast_pool -> NewMethodDeclaration();
    p -> modifiers_opt = MakeModifiers();
    p -> type = DYNAMIC_CAST<AstType*> (Sym(3));
    AstMethodDeclarator* q = ast_pool -> NewMethodDeclarator();
    q -> identifier_token = Token(4);
    q -> left_parenthesis_token = Token(5);
    q -> right_parenthesis_token = Token(6);
    p -> method_declarator = q;
    p -> default_value_opt = DYNAMIC_CAST<AstMemberValue*> (Sym(7));
    p -> semicolon_token_opt = Token(8);
    Sym(1) = p;
}
./

--
-- Annotations were added in JSR 175. We must inline expand Modifiersopt to
-- avoid ambiguity.
--
AnnotationTypeMemberDeclaration ::= Modifiers Marker Type 'Identifier' '(' ')'
                                    DefaultValueopt ';'
\:$MakeAnnotationTypeMemberDeclaration:\
/.$shared_function
//
// void MakeAnnotationTypeMemberDeclaration();
//./

--
-- Annotations were added in JSR 175.
--
AnnotationTypeMemberDeclaration ::= ConstantDeclaration
\:$NoAction:\
/.$shared_NoAction./

--
-- Annotations were added in JSR 175. Consolidate and simplify.
--
--AnnotationTypeMemberDeclaration ::= ClassDeclaration
--AnnotationTypeMemberDeclaration ::= InterfaceDeclaration
--AnnotationTypeMemberDeclaration ::= EnumDeclaration
--AnnotationTypeMemberDeclaration ::= AnnotationTypeDeclaration
--AnnotationTypeMemberDeclaration ::= ';'
AnnotationTypeMemberDeclaration ::= TypeDeclaration
\:$NoAction:\
/.$shared_NoAction./

--
-- Annotations were added in JSR 175.
--
DefaultValue ::= 'default' MemberValue
\:$SetSym1ToSym2:\
/.$shared_function
//
// void SetSym1ToSym2();
//./

--
-- Annotations were added in JSR 175.
--
DefaultValueopt ::= $empty
\:$NullAction:\
/.$shared_NullAction./

--
-- Annotations were added in JSR 175.
--
DefaultValueopt ::= DefaultValue
\:$NoAction:\
/.$shared_NoAction./

--18.10 Productions from 10: Arrays
--
-- The rule VariableInitializersopt was expanded inline below to make the
-- grammar LALR(1). The use of Marker allows us to share code.
--
-- ArrayInitializer ::= '{' VariableInitializersopt ,opt '}'
ArrayInitializer ::= '{' Marker ,opt '}'
\:$MakeArrayInitializer:\
/.$shared_function
//
// void MakeArrayInitializer();
//./

ArrayInitializer ::= '{' VariableInitializers ,opt '}'
\:$MakeArrayInitializer:\
/.$shared_function
//
// void MakeArrayInitializer();
//./

VariableInitializers ::= VariableInitializer
\:$StartList:\
/.$shared_StartList./

VariableInitializers ::= VariableInitializers ',' VariableInitializer
\:$AddList3:\
/.$shared_AddList3./

--18.11 Productions from 13: Blocks and Statements

Block ::= '{' BlockStatementsopt '}'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstBlock* p = ast_pool -> NewBlock();
    p -> left_brace_token = Token(1);
    if (Sym(2))
    {
        AstListNode* tail = DYNAMIC_CAST<AstListNode*> (Sym(2));
        p -> AllocateStatements(tail -> index + 1);
        AstListNode* root = tail;
        do
        {
            root = root -> next;
            p -> AddStatement(DYNAMIC_CAST<AstStatement*> (root -> element));
        } while (root != tail);
        FreeCircularList(tail);
    }
    p -> right_brace_token = Token(3);
    Sym(1) = p;
}
./

BlockStatements ::= BlockStatement
\:$StartList:\
/.$shared_StartList./

BlockStatements ::= BlockStatements BlockStatement
\:$AddList2:\
/.$shared_AddList2./

BlockStatementsopt ::= $empty
\:$NullAction:\
/.$shared_NullAction./

BlockStatementsopt ::= BlockStatements
\:$NoAction:\
/.$shared_NoAction./

BlockStatement ::= LocalVariableDeclarationStatement
\:$NoAction:\
/.$shared_NoAction./

BlockStatement ::= Statement
\:$NoAction:\
/.$shared_NoAction./

--1.1 feature
BlockStatement ::= ClassDeclaration
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    Sym(1) = ast_pool ->
        NewLocalClassStatement(DYNAMIC_CAST<AstClassDeclaration*> (Sym(1)));
}
./

--
-- Enums were added in JSR 201.
--
BlockStatement ::= EnumDeclaration
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    Sym(1) = ast_pool ->
        NewLocalClassStatement(DYNAMIC_CAST<AstEnumDeclaration*> (Sym(1)));
}
./

--
-- NOTE: This rule is not in the original grammar. We added it, and changed
-- the rule for ConstructorBody, in order to issue a nicer error message
-- when this() or super() is encountered out of context.
--
BlockStatement ::= ExplicitConstructorInvocation
\:$NoAction:\
/.$shared_NoAction./

LocalVariableDeclarationStatement ::= LocalVariableDeclaration ';'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    DYNAMIC_CAST<AstLocalVariableStatement*> (Sym(1)) -> semicolon_token_opt =
        Token(2);
}
./

--
-- For nicer error messages, we accept all modifiers, even though only
-- 'final' and annotations are valid. Also, we must inline expand finalopt
-- to avoid ambiguity. The use of Marker allows us to share code.
--
--LocalVariableDeclaration ::= finalopt Type VariableDeclarators
LocalVariableDeclaration ::= Type Marker Marker VariableDeclarators
\:$MakeLocalVariable:\
/.$location
void Parser::MakeLocalVariable()
{
    AstLocalVariableStatement* p = ast_pool -> NewLocalVariableStatement();
    if (Sym(2))
    {
        p -> modifiers_opt = MakeModifiers();
        p -> type = DYNAMIC_CAST<AstType*> (Sym(2));
    }
    else p -> type = DYNAMIC_CAST<AstType*> (Sym(1));
    AstListNode* tail = DYNAMIC_CAST<AstListNode*> (Sym(4));
    p -> AllocateVariableDeclarators(tail -> index + 1);
    AstListNode* root = tail;
    do
    {
        root = root -> next;
        p -> AddVariableDeclarator(DYNAMIC_CAST<AstVariableDeclarator*>
                                   (root -> element));
    } while (root != tail);
    FreeCircularList(tail);
    Sym(1) = p;
}
./

--1.1 feature
--
-- For nicer error messages, we accept all modifiers, even though only
-- 'final' and annotations are valid. Also, we must inline expand finalopt
-- to avoid ambiguity. The use of Marker allows us to share code.
--
--LocalVariableDeclaration ::= final Type VariableDeclarators
LocalVariableDeclaration ::= Modifiers Type Marker VariableDeclarators
\:$MakeLocalVariable:\
/.$shared_function
//
// void MakeLocalVariable();
//./

Statement ::= StatementWithoutTrailingSubstatement
\:$NoAction:\
/.$shared_NoAction./

Statement ::= LabeledStatement
\:$NoAction:\
/.$shared_NoAction./

Statement ::= IfThenStatement
\:$NoAction:\
/.$shared_NoAction./

Statement ::= IfThenElseStatement
\:$NoAction:\
/.$shared_NoAction./

Statement ::= WhileStatement
\:$NoAction:\
/.$shared_NoAction./

Statement ::= ForStatement
\:$NoAction:\
/.$shared_NoAction./

--
-- Enhanced for statements (also known as foreach, but without a new keyword),
-- were added in JDK 1.5, as part of JSR 201.
--
Statement ::= ForeachStatement
\:$NoAction:\
/.$shared_NoAction./

StatementNoShortIf ::= StatementWithoutTrailingSubstatement
\:$NoAction:\
/.$shared_NoAction./

StatementNoShortIf ::= LabeledStatementNoShortIf
\:$NoAction:\
/.$shared_NoAction./

StatementNoShortIf ::= IfThenElseStatementNoShortIf
\:$NoAction:\
/.$shared_NoAction./

StatementNoShortIf ::= WhileStatementNoShortIf
\:$NoAction:\
/.$shared_NoAction./

StatementNoShortIf ::= ForStatementNoShortIf
\:$NoAction:\
/.$shared_NoAction./

--
-- Enhanced for statements (also known as foreach, but without a new keyword),
-- were added in JDK 1.5, as part of JSR 201.
--
StatementNoShortIf ::= ForeachStatementNoShortIf
\:$NoAction:\
/.$shared_NoAction./

StatementWithoutTrailingSubstatement ::= Block
\:$NoAction:\
/.$shared_NoAction./

StatementWithoutTrailingSubstatement ::= EmptyStatement
\:$NoAction:\
/.$shared_NoAction./

StatementWithoutTrailingSubstatement ::= ExpressionStatement
\:$NoAction:\
/.$shared_NoAction./

StatementWithoutTrailingSubstatement ::= SwitchStatement
\:$NoAction:\
/.$shared_NoAction./

StatementWithoutTrailingSubstatement ::= DoStatement
\:$NoAction:\
/.$shared_NoAction./

StatementWithoutTrailingSubstatement ::= BreakStatement
\:$NoAction:\
/.$shared_NoAction./

StatementWithoutTrailingSubstatement ::= ContinueStatement
\:$NoAction:\
/.$shared_NoAction./

StatementWithoutTrailingSubstatement ::= ReturnStatement
\:$NoAction:\
/.$shared_NoAction./

StatementWithoutTrailingSubstatement ::= SynchronizedStatement
\:$NoAction:\
/.$shared_NoAction./

StatementWithoutTrailingSubstatement ::= ThrowStatement
\:$NoAction:\
/.$shared_NoAction./

StatementWithoutTrailingSubstatement ::= TryStatement
\:$NoAction:\
/.$shared_NoAction./

StatementWithoutTrailingSubstatement ::= AssertStatement
\:$NoAction:\
/.$shared_NoAction./

EmptyStatement ::= ';'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    Sym(1) = ast_pool -> NewEmptyStatement(Token(1));
}
./

LabeledStatement ::= 'Identifier' ':' Statement
\:$MakeLabeledStatement:\
/.$location
void Parser::MakeLabeledStatement()
{
    AstBlock* p = Sym(3) -> BlockCast();

    if (! p || p -> label_opt)
    {
        //
        // When a statement is labeled, it is enclosed in a block.
        // This is necessary in order to allow the same name to be
        // reused to label a subsequent statement at the same nesting
        // level... See ProcessBlock, ProcessStatement,...
        //
        p = ast_pool -> GenBlock();
        p -> AllocateStatements(1); // allocate 1 element
        p -> left_brace_token = Token(1);
        p -> AddStatement(DYNAMIC_CAST<AstStatement*> (Sym(3)));
        p -> right_brace_token = Sym(3) -> RightToken();
    }

    p -> label_opt = Token(1); // add label to statement
    Sym(1) = p; // The final result is a block containing the labeled-statement
}
./

LabeledStatementNoShortIf ::= 'Identifier' ':' StatementNoShortIf
\:$MakeLabeledStatement:\
/.$shared_function
//
// void MakeLabeledStatement();
//./

ExpressionStatement ::= StatementExpression ';'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    DYNAMIC_CAST<AstExpressionStatement*> (Sym(1)) -> semicolon_token_opt =
        Token(2);
}
./

StatementExpression ::= Assignment
\:$MakeExpressionStatement:\
/.$location
void Parser::MakeExpressionStatement()
{
    AstExpressionStatement* p = ast_pool -> NewExpressionStatement();
    p -> expression = DYNAMIC_CAST<AstExpression*> (Sym(1));
    Sym(1) = p;
}
./

StatementExpression ::= PreIncrementExpression
\:$MakeExpressionStatement:\
/.$shared_function
//
// void MakeExpressionStatement();
//./

StatementExpression ::= PreDecrementExpression
\:$MakeExpressionStatement:\
/.$shared_function
//
// void MakeExpressionStatement();
//./

StatementExpression ::= PostIncrementExpression
\:$MakeExpressionStatement:\
/.$shared_function
//
// void MakeExpressionStatement();
//./

StatementExpression ::= PostDecrementExpression
\:$MakeExpressionStatement:\
/.$shared_function
//
// void MakeExpressionStatement();
//./

StatementExpression ::= MethodInvocation
\:$MakeExpressionStatement:\
/.$shared_function
//
// void MakeExpressionStatement();
//./

StatementExpression ::= ClassInstanceCreationExpression
\:$MakeExpressionStatement:\
/.$shared_function
//
// void MakeExpressionStatement();
//./

--
-- The use of Marker allows us to share code.
--
--IfThenStatement ::= 'if' '(' Expression ')' Statement
IfThenStatement ::= 'if' '(' Expression ')' Statement Marker Marker
\:$MakeIfThenElseStatement:\
/.$location
void Parser::MakeIfThenElseStatement()
{
    //
    // We wrap the true and false statements in a block, to make the semantic
    // pass easier.
    //
    AstIfStatement* p = ast_pool -> NewIfStatement();
    p -> if_token = Token(1);
    p -> expression = DYNAMIC_CAST<AstExpression*> (Sym(3));
    p -> true_statement = MakeBlock(5);
    p -> false_statement_opt = Sym(7) ? MakeBlock(7) : NULL;
    Sym(1) = p;
}

//
// Ensures the symbol at tokennum is an AstBlock (wrapping it in a generated
// block if necessary).
//
AstBlock* Parser::MakeBlock(int tokennum)
{
    AstBlock* block = Sym(tokennum) -> BlockCast();
    if (! block)
    {
        block = ast_pool -> GenBlock();
        block -> AllocateStatements(1); // allocate 1 element
        block -> left_brace_token = Token(tokennum);
        block -> AddStatement(DYNAMIC_CAST<AstStatement*> (Sym(tokennum)));
        block -> right_brace_token = Sym(tokennum) -> RightToken();
    }
    return block;
}
./

IfThenElseStatement ::= 'if' '(' Expression ')' StatementNoShortIf
                        'else' Statement
\:$MakeIfThenElseStatement:\
/.$shared_function
//
// void MakeIfThenElseStatement();
//./

IfThenElseStatementNoShortIf ::= 'if' '(' Expression ')' StatementNoShortIf
                                 'else' StatementNoShortIf
\:$MakeIfThenElseStatement:\
/.$shared_function
//
// void MakeIfThenElseStatement();
//./

SwitchStatement ::= 'switch' '(' Expression ')' SwitchBlock
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstSwitchStatement* p = DYNAMIC_CAST<AstSwitchStatement*> (Sym(5));
    p -> switch_token = Token(1);
    p -> expression = DYNAMIC_CAST<AstExpression*> (Sym(3));
    Sym(1) = p;
}
./

--
-- To avoid ambiguity with consecutive optional items, and to special
-- case trailing labels, we expand this inline.
--
--SwitchBlock ::= '{' SwitchBlockStatementsopt SwitchLabelsopt '}'
--
SwitchBlock ::= '{' SwitchBlockStatements SwitchLabelsopt '}'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstSwitchStatement* p = ast_pool -> NewSwitchStatement();
    AstBlock* block = ast_pool -> NewBlock();
    block -> left_brace_token = Token(1);
    AstListNode* tail = DYNAMIC_CAST<AstListNode*> (Sym(2));
    block -> AllocateStatements(tail -> index + (Sym(3) ? 2 : 1));
    AstListNode* root = tail;
    do
    {
        root = root -> next;
        block -> AddStatement(DYNAMIC_CAST<AstStatement*> (root -> element));
    } while (root != tail);
    FreeCircularList(tail);
    if (Sym(3))
        block -> AddStatement
            (MakeSwitchBlockStatement(DYNAMIC_CAST<AstListNode*> (Sym(3))));
    block -> right_brace_token = Token(4);
    block -> SetTag(AstBlock::SWITCH);
    p -> switch_block = block;
    Sym(1) = p;
}

//
// Creates an AstSwitchBlockStatement from the given non-null labels, and
// possibly null list of statements.
//
AstStatement* Parser::MakeSwitchBlockStatement(AstListNode* labels,
                                               AstListNode* statements)
{
    AstSwitchBlockStatement* p = ast_pool -> NewSwitchBlockStatement();
    assert(labels);
    AstListNode* tail = labels;
    p -> AllocateSwitchLabels(tail -> index + 1);
    AstListNode* root = tail;
    do
    {
        root = root -> next;
        p -> AddSwitchLabel(DYNAMIC_CAST<AstSwitchLabel*> (root -> element));
    } while (root != tail);
    FreeCircularList(tail);
    if (statements)
    {
        tail = statements;
        p -> AllocateStatements(tail -> index + 1);
        root = tail;
        do
        {
            root = root -> next;
            p -> AddStatement(DYNAMIC_CAST<AstStatement*> (root -> element));
        } while (root != tail);
        FreeCircularList(tail);
    }
    else
    {
        p -> AllocateStatements(1);
        p -> AddStatement(ast_pool -> GenEmptyStatement(labels ->
                                                        RightToken()));
    }
    p -> right_brace_token =
        p -> Statement(p -> NumStatements() - 1) -> RightToken();
    return p;
}
./

SwitchBlock ::= '{' SwitchLabelsopt '}'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstSwitchStatement* p = ast_pool -> NewSwitchStatement();
    AstBlock* block = ast_pool -> NewBlock();
    block -> AllocateStatements(1); // allocate 1 element
    block -> left_brace_token = Token(1);
    if (Sym(2))
        block -> AddStatement
            (MakeSwitchBlockStatement(DYNAMIC_CAST<AstListNode*> (Sym(2))));
    block -> right_brace_token = Token(3);
    block -> SetTag(AstBlock::SWITCH);

    p -> switch_block = block;

    Sym(1) = p;
}
./

SwitchBlockStatements ::= SwitchBlockStatement
\:$StartList:\
/.$shared_StartList./

SwitchBlockStatements ::= SwitchBlockStatements SwitchBlockStatement
\:$AddList2:\
/.$shared_AddList2./

SwitchBlockStatement ::= SwitchLabels BlockStatements
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    Sym(1) = MakeSwitchBlockStatement(DYNAMIC_CAST<AstListNode*> (Sym(1)),
                                      DYNAMIC_CAST<AstListNode*> (Sym(2)));
}
./

SwitchLabels ::= SwitchLabel
\:$StartList:\
/.$shared_StartList./

SwitchLabels ::= SwitchLabels SwitchLabel
\:$AddList2:\
/.$shared_AddList2./

SwitchLabelsopt ::= $empty
\:$NullAction:\
/.$shared_NullAction./

SwitchLabelsopt ::= SwitchLabels
\:$NoAction:\
/.$shared_NoAction./

--
-- Simplify.
--
--SwitchLabel ::= 'case' ConstantExpression ':'
SwitchLabel ::= 'case' Expression ':'
\:$MakeSwitchLabel:\
/.$location
void Parser::MakeSwitchLabel()
{
    AstSwitchLabel* p = ast_pool -> NewSwitchLabel();
    p -> case_token = Token(1);
    p -> expression_opt = DYNAMIC_CAST<AstExpression*> (Sym(2));
    p -> colon_token = Token(3);
    Sym(1) = p;
}
./

--
-- The use of Marker allows us to share code.
--
--SwitchLabel ::= 'default' ':'
SwitchLabel ::= 'default' Marker ':'
\:$MakeSwitchLabel:\
/.$shared_function
//
// void MakeSwitchLabel();
//./

WhileStatement ::= 'while' '(' Expression ')' Statement
\:$MakeWhileStatement:\
/.$location
void Parser::MakeWhileStatement()
{
    //
    // We wrap the loop statement in a block, to make the semantic pass easier.
    //
    AstWhileStatement* p = ast_pool -> NewWhileStatement();
    p -> while_token = Token(1);
    p -> expression = DYNAMIC_CAST<AstExpression*> (Sym(3));
    p -> statement = MakeBlock(5);

    //
    // We also wrap the loop in a block, to make the semantic pass easier.
    //
    AstBlock* block = ast_pool -> GenBlock();
    block -> AllocateStatements(1); // allocate 1 element
    block -> left_brace_token = Token(1);
    block -> AddStatement(p);
    block -> right_brace_token = Sym(5) -> RightToken();
    Sym(1) = block;
}
./

WhileStatementNoShortIf ::= 'while' '(' Expression ')' StatementNoShortIf
\:$MakeWhileStatement:\
/.$shared_function
//
// void MakeWhileStatement();
//./

DoStatement ::= 'do' Statement 'while' '(' Expression ')' ';'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    //
    // We wrap the loop statement in a block, to make the semantic pass easier.
    //
    AstDoStatement* p = ast_pool -> NewDoStatement();
    p -> do_token = Token(1);
    p -> statement = MakeBlock(2);
    p -> while_token = Token(3);
    p -> expression = DYNAMIC_CAST<AstExpression*> (Sym(5));
    p -> semicolon_token = Token(7);

    //
    // We also wrap the loop in a block, to make the semantic pass easier.
    //
    AstBlock* block = ast_pool -> GenBlock();
    block -> AllocateStatements(1); // allocate 1 element
    block -> left_brace_token = Token(1);
    block -> AddStatement(p);
    block -> right_brace_token = Token(7);
    Sym(1) = block;
}
./

ForStatement ::= 'for' '(' ForInitopt ';' Expressionopt ';' ForUpdateopt ')'
                 Statement
\:$MakeForStatement:\
/.$location
void Parser::MakeForStatement()
{
    //
    // We wrap the loop statement in a block, to make the semantic pass easier.
    //
    AstForStatement* p = ast_pool -> NewForStatement();
    p -> for_token = Token(1);
    if (Sym(3))
    {
        AstListNode* tail = DYNAMIC_CAST<AstListNode*> (Sym(3));
        p -> AllocateForInitStatements(tail -> index + 1);
        AstListNode* root = tail;
        do
        {
            root = root -> next;
            p -> AddForInitStatement(DYNAMIC_CAST<AstStatement*>
                                     (root -> element));
        } while (root != tail);
        FreeCircularList(tail);
    }
    p -> end_expression_opt = DYNAMIC_CAST<AstExpression*> (Sym(5));
    if (Sym(7))
    {
        AstListNode* tail = DYNAMIC_CAST<AstListNode*> (Sym(7));
        p -> AllocateForUpdateStatements(tail -> index + 1);
        AstListNode* root = tail;
        do
        {
            root = root -> next;
            p -> AddForUpdateStatement(DYNAMIC_CAST<AstExpressionStatement*>
                                       (root -> element));
        } while (root != tail);
        FreeCircularList(tail);
    }
    p -> statement = MakeBlock(9);

    //
    // We also wrap the loop in a block, to make the semantic pass easier. In
    // particular, this lets us correctly handle "for(int i;;);for(int i;;);".
    //
    AstBlock* block = ast_pool -> NewBlock();
    block -> AllocateStatements(1); // allocate 1 element
    block -> left_brace_token = Token(1);
    block -> AddStatement(p);
    block -> right_brace_token = Sym(9) -> RightToken();
    block -> no_braces = true;
    Sym(1) = block;
}
./

ForStatementNoShortIf ::= 'for' '(' ForInitopt ';' Expressionopt ';'
                          ForUpdateopt ')' StatementNoShortIf
\:$MakeForStatement:\
/.$shared_function
//
// void MakeForStatement();
//./

ForInit ::= StatementExpressionList
\:$NoAction:\
/.$shared_NoAction./

ForInit ::= LocalVariableDeclaration
\:$StartList:\
/.$shared_StartList./

ForInitopt ::= $empty
\:$NullAction:\
/.$shared_NullAction./

ForInitopt ::= ForInit
\:$NoAction:\
/.$shared_NoAction./

ForUpdate ::= StatementExpressionList
\:$NoAction:\
/.$shared_NoAction./

ForUpdateopt ::= $empty
\:$NullAction:\
/.$shared_NullAction./

ForUpdateopt ::= ForUpdate
\:$NoAction:\
/.$shared_NoAction./

StatementExpressionList ::= StatementExpression
\:$StartList:\
/.$shared_StartList./

StatementExpressionList ::= StatementExpressionList ',' StatementExpression
\:$AddList3:\
/.$shared_AddList3./

ForeachStatement ::= 'for' '(' FormalParameter ':' Expression ')' Statement
\:$MakeForeachStatement:\
/.$location
void Parser::MakeForeachStatement()
{
    //
    // We wrap the loop statement in a block, to make the semantic pass easier.
    //
    AstForeachStatement* p = ast_pool -> NewForeachStatement();
    p -> for_token = Token(1);
    p -> formal_parameter = DYNAMIC_CAST<AstFormalParameter*> (Sym(3));
    p -> expression = DYNAMIC_CAST<AstExpression*> (Sym(5));
    p -> statement = MakeBlock(7);

    //
    // We also wrap the loop in a block, to make the semantic pass easier. In
    // particular, this lets us correctly handle
    // "for(int i:new int[0]);for(int i::new int[0]);".
    //
    AstBlock* block = ast_pool -> NewBlock();
    block -> AllocateStatements(1); // allocate 1 element
    block -> left_brace_token = Token(1);
    block -> AddStatement(p);
    block -> right_brace_token = Sym(7) -> RightToken();
    block -> no_braces = true;
    Sym(1) = block;
}
./

ForeachStatementNoShortIf ::= 'for' '(' FormalParameter ':' Expression ')'
                              StatementNoShortIf
\:$MakeForeachStatement:\
/.$shared_function
//
// void MakeForeachStatement();
//./

--
-- Assert statements were added in JDK 1.4, as part of JSR 41.
-- The use of Marker allows us to share code.
--
--AssertStatement ::= 'assert' Expression ';'
AssertStatement ::= 'assert' Expression Marker Marker ';'
\:$MakeAssertStatement:\
/.$location
void Parser::MakeAssertStatement()
{
    AstAssertStatement* p = ast_pool -> NewAssertStatement();
    p -> assert_token = Token(1);
    p -> condition = DYNAMIC_CAST<AstExpression*> (Sym(2));
    p -> message_opt = DYNAMIC_CAST<AstExpression*> (Sym(4));
    p -> semicolon_token = Token(5);
    Sym(1) = p;
}
./

AssertStatement ::= 'assert' Expression ':' Expression ';'
\:$MakeAssertStatement:\
/.$shared_function
//
// void MakeAssertStatement();
//./

BreakStatement ::= 'break' Identifieropt ';'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstBreakStatement* p = ast_pool -> NewBreakStatement();
    p -> break_token = Token(1);
    if (Token(3) > Token(2))
        p -> identifier_token_opt = Token(2);
    p -> semicolon_token = Token(3);
    Sym(1) = p;
}
./

ContinueStatement ::= 'continue' Identifieropt ';'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstContinueStatement* p = ast_pool -> NewContinueStatement();
    p -> continue_token = Token(1);
    if (Token(3) > Token(2))
        p -> identifier_token_opt = Token(2);
    p -> semicolon_token = Token(3);
    Sym(1) = p;
}
./

ReturnStatement ::= 'return' Expressionopt ';'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstReturnStatement* p = ast_pool -> NewReturnStatement();
    p -> return_token = Token(1);
    p -> expression_opt = DYNAMIC_CAST<AstExpression*> (Sym(2));
    p -> semicolon_token = Token(3);
    Sym(1) = p;
}
./

ThrowStatement ::= 'throw' Expression ';'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstThrowStatement* p = ast_pool -> NewThrowStatement();
    p -> throw_token = Token(1);
    p -> expression = DYNAMIC_CAST<AstExpression*> (Sym(2));
    p -> semicolon_token = Token(3);
    Sym(1) = p;
}
./

SynchronizedStatement ::= 'synchronized' '(' Expression ')' Block
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstSynchronizedStatement* p = ast_pool -> NewSynchronizedStatement();
    p -> synchronized_token = Token(1);
    p -> expression = DYNAMIC_CAST<AstExpression*> (Sym(3));
    p -> block = DYNAMIC_CAST<AstBlock*> (Sym(5));
    p -> block -> SetTag(AstBlock::SYNCHRONIZED);

    Sym(1) = p;
}
./

--
-- The use of Marker allows us to share code.
--
--TryStatement ::= 'try' Block Catches
TryStatement ::= 'try' Block Catches Marker
\:$MakeTryStatement:\
/.$location
void Parser::MakeTryStatement()
{
    AstTryStatement* p = ast_pool -> NewTryStatement();
    p -> try_token = Token(1);
    p -> block = DYNAMIC_CAST<AstBlock*> (Sym(2));
    if (Sym(3))
    {
        AstListNode* tail = DYNAMIC_CAST<AstListNode*> (Sym(3));
        p -> AllocateCatchClauses(tail -> index + 1);
        AstListNode* root = tail;
        do
        {
            root = root -> next;
            p -> AddCatchClause(DYNAMIC_CAST<AstCatchClause*>
                                (root -> element));
        } while (root != tail);
        FreeCircularList(tail);
    }
    if (Sym(4))
    {
        p -> block -> SetTag(AstBlock::TRY_CLAUSE_WITH_FINALLY);
        for (unsigned i = 0; i < p -> NumCatchClauses(); i++)
            p -> CatchClause(i) -> block ->
                SetTag(AstBlock::TRY_CLAUSE_WITH_FINALLY);
        p -> finally_clause_opt = DYNAMIC_CAST<AstFinallyClause*> (Sym(4));
    }
    Sym(1) = p;
}
./

TryStatement ::= 'try' Block Catchesopt Finally
\:$MakeTryStatement:\
/.$shared_function
//
// void MakeTryStatement();
//./

Catches ::= CatchClause
\:$StartList:\
/.$shared_StartList./

Catches ::= Catches CatchClause
\:$AddList2:\
/.$shared_AddList2./

Catchesopt ::= $empty
\:$NullAction:\
/.$shared_NullAction./

Catchesopt ::= Catches
\:$NoAction:\
/.$shared_NoAction./

CatchClause ::= 'catch' '(' FormalParameter ')' Block
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstCatchClause* p = ast_pool -> NewCatchClause();
    p -> catch_token = Token(1);
    p -> formal_parameter = DYNAMIC_CAST<AstFormalParameter*> (Sym(3));
    p -> block = DYNAMIC_CAST<AstBlock*> (Sym(5));

    Sym(1) = p;
}
./

Finally ::= 'finally' Block
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstFinallyClause* p = ast_pool -> NewFinallyClause();
    p -> finally_token = Token(1);
    p -> block = DYNAMIC_CAST<AstBlock*> (Sym(2));
    p -> block -> SetTag(AstBlock::FINALLY);

    Sym(1) = p;
}
./

--18.12 Productions from 14: Expressions

Primary ::= PrimaryNoNewArray
\:$NoAction:\
/.$shared_NoAction./

--1.2 feature
--
-- It is legal to access an element of an initialized array, as in
-- new int[] {0}[0]; this requires splitting the original rule for
-- array creation into two.
--
--Primary ::= ArrayCreationExpression
Primary ::= ArrayCreationUninitialized
\:$NoAction:\
/.$shared_NoAction./

Primary ::= ArrayCreationInitialized
\:$NoAction:\
/.$shared_NoAction./

PrimaryNoNewArray ::= Literal
\:$NoAction:\
/.$shared_NoAction./

PrimaryNoNewArray ::= 'this'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    Sym(1) = ast_pool -> NewThisExpression(Token(1));
}
./

--
-- We split this into two rules to allow better parsing of parenthesized
-- expressions vs. casts.  All expressions have a dual *NotName form, so that
-- the decision of whether "(name)" starts a cast or is a primary does not
-- cause parsing ambiguities. The use of Marker allows us to share code.
-- Also note that splitting this rule aids in parsing generics.
--
--PrimaryNoNewArray ::= '(' Expression ')'
PrimaryNoNewArray ::= '(' Name Marker ')'
\:$MakeParenthesizedExpression:\
/.$location
void Parser::MakeParenthesizedExpression()
{
    AstParenthesizedExpression* p = ast_pool -> NewParenthesizedExpression();
    p -> left_parenthesis_token = Token(1);
    p -> expression = DYNAMIC_CAST<AstExpression*> (Sym(2));
    p -> right_parenthesis_token = Token(4);
    Sym(1) = p;
}
./

--
-- The use of Marker allows us to share code.
--
PrimaryNoNewArray ::= '(' ExpressionNotName Marker ')'
\:$MakeParenthesizedExpression:\
/.$shared_function
//
// void MakeParenthesizedExpression();
//./

PrimaryNoNewArray ::= ClassInstanceCreationExpression
\:$NoAction:\
/.$shared_NoAction./

PrimaryNoNewArray ::= FieldAccess
\:$NoAction:\
/.$shared_NoAction./

--1.1 feature
--
-- Note that we had to rework this to avoid ambiguity
--
--PrimaryNoNewArray ::= ClassType '.' 'this'
PrimaryNoNewArray ::= Name '.' 'this'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstThisExpression* p = ast_pool -> NewThisExpression(Token(3));
    p -> base_opt = ast_pool -> NewTypeName(DYNAMIC_CAST<AstName*> (Sym(1)));
    Sym(1) = p;
}
./

--1.1 feature
--
-- Note that we had to rework this to avoid ambiguity.
--
--PrimaryNoNewArray ::= Type '.' 'class'
PrimaryNoNewArray ::= PrimitiveType Dimsopt '.' 'class'
\:$MakeClassLiteral:\
/.$location
void Parser::MakeClassLiteral()
{
    AstClassLiteral* p = ast_pool -> NewClassLiteral(Token(4));
    if (Token(3) == Token(4))
        Sym(2) = NULL;
    p -> type = MakeArrayType(1);
    Sym(1) = p;
}
./

PrimaryNoNewArray ::= Name Dims '.' 'class'
\:$MakeClassLiteral:\
/.$shared_function
//
// void MakeClassLiteral();
//./

--
-- The use of Marker allows us to share code.
--
--PrimaryNoNewArray ::= Name '.' 'class'
PrimaryNoNewArray ::= Name '.' Marker 'class'
\:$MakeClassLiteral:\
/.$shared_function
//
// void MakeClassLiteral();
//./

--
-- The use of Marker allows us to share code.
--
--PrimaryNoNewArray ::= 'void' '.' 'class'
PrimaryNoNewArray ::= VoidType '.' Marker 'class'
\:$MakeClassLiteral:\
/.$shared_function
//
// void MakeClassLiteral();
//./

PrimaryNoNewArray ::= MethodInvocation
\:$NoAction:\
/.$shared_NoAction./

PrimaryNoNewArray ::= ArrayAccess
\:$NoAction:\
/.$shared_NoAction./

--1.1 feature
--
-- In Java 1.0 a ClassBody could not appear at all in a
-- ClassInstanceCreationExpression. Simplify.
--
--ClassInstanceCreationExpression ::= 'new' ClassOrInterfaceType '('
--                                    ArgumentListopt ')' ClassBodyopt
ClassInstanceCreationExpression ::= 'new' ClassOrInterfaceType Arguments
                                    ClassBodyopt
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstClassCreationExpression* p = ast_pool -> NewClassCreationExpression();
    p -> new_token = Token(1);
    p -> class_type = DYNAMIC_CAST<AstTypeName*> (Sym(2));
    p -> arguments = DYNAMIC_CAST<AstArguments*> (Sym(3));
    p -> class_body_opt = DYNAMIC_CAST<AstClassBody*> (Sym(4));
    if (p -> class_body_opt)
        p -> class_body_opt -> identifier_token =
            p -> class_type -> IdentifierToken();
    Sym(1) = p;
}
./

--
-- Parameterized types were added in JSR 14.
--
--ClassInstanceCreationExpression ::= 'new' ClassOrInterfaceType '('
--                                    ArgumentListopt ')' ClassBodyopt
ClassInstanceCreationExpression ::= 'new' TypeArguments ClassOrInterfaceType
                                    Arguments ClassBodyopt
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstClassCreationExpression* p = ast_pool -> NewClassCreationExpression();
    p -> new_token = Token(1);
    p -> type_arguments_opt = MakeExplicitTypeArguments(2);
    p -> class_type = DYNAMIC_CAST<AstTypeName*> (Sym(3));
    p -> arguments = DYNAMIC_CAST<AstArguments*> (Sym(4));
    p -> class_body_opt = DYNAMIC_CAST<AstClassBody*> (Sym(5));
    if (p -> class_body_opt)
        p -> class_body_opt -> identifier_token =
            p -> class_type -> IdentifierToken();
    Sym(1) = p;
}
./

--1.1 feature
--
-- Parameterized types were added in JSR 14.
--
--ClassInstanceCreationExpression ::= Primary '.' 'new' 'Identifier' '('
--                                    ArgumentListopt ')' ClassBodyopt
ClassInstanceCreationExpression ::= Primary '.' 'new' TypeArgumentsopt
                                    'Identifier' TypeArgumentsopt Arguments
                                    ClassBodyopt
\:$MakeQualifiedNew:\
/.$location
void Parser::MakeQualifiedNew()
{
    AstClassCreationExpression* p = ast_pool -> NewClassCreationExpression();
    p -> base_opt = DYNAMIC_CAST<AstExpression*> (Sym(1));
    p -> new_token = Token(3);
    p -> type_arguments_opt = MakeExplicitTypeArguments(4);
    p -> class_type = ast_pool -> NewTypeName(MakeSimpleName(5));
    p -> class_type -> type_arguments_opt = MakeExplicitTypeArguments(6);
    p -> arguments = DYNAMIC_CAST<AstArguments*> (Sym(7));
    p -> class_body_opt = DYNAMIC_CAST<AstClassBody*> (Sym(8));
    if (p -> class_body_opt)
        p -> class_body_opt -> identifier_token = Token(5);
    Sym(1) = p;
}
./

--1.1 feature
--
-- Parameterized types were added in JSR 14.
--
--ClassInstanceCreationExpression ::= Name '.' 'new' 'Identifier' '('
--                                    ArgumentListopt ')' ClassBodyopt
ClassInstanceCreationExpression ::= Name '.' 'new' TypeArgumentsopt
                                    'Identifier' TypeArgumentsopt Arguments
                                    ClassBodyopt
\:$MakeQualifiedNew:\
/.$shared_function
//
// void MakeQualifiedNew();
//./

ArgumentList ::= Expression
\:$StartList:\
/.$shared_StartList./

ArgumentList ::= ArgumentList ',' Expression
\:$AddList3:\
/.$shared_AddList3./

ArgumentListopt ::= $empty
\:$NullAction:\
/.$shared_NullAction./

ArgumentListopt ::= ArgumentList
\:$NoAction:\
/.$shared_NoAction./

--1.2 feature
--
-- ArrayCreationExpression is split into two parsing categories, to
-- allow array access on an initialized array.  See above.
--
--ArrayCreationExpression ::= 'new' PrimitiveType DimExprs Dimsopt
ArrayCreationUninitialized ::= 'new' PrimitiveType DimExprs Dimsopt
\:$MakeArrayCreationUninitialized:\
/.$location
void Parser::MakeArrayCreationUninitialized()
{
    AstArrayCreationExpression* p = ast_pool -> NewArrayCreationExpression();
    p -> new_token = Token(1);
    p -> array_type = DYNAMIC_CAST<AstType*> (Sym(2));
    AstListNode* tail = DYNAMIC_CAST<AstListNode*> (Sym(3));
    p -> AllocateDimExprs(tail -> index + 1);
    AstListNode* root = tail;
    do
    {
        root = root -> next;
        p -> AddDimExpr(DYNAMIC_CAST<AstDimExpr*> (root -> element));
    } while (root != tail);
    FreeCircularList(tail);
    p -> brackets_opt = DYNAMIC_CAST<AstBrackets*> (Sym(4));
    Sym(1) = p;
}
./

--ArrayCreationExpression ::= 'new' ClassOrInterfaceType DimExprs Dimsopt
ArrayCreationUninitialized ::= 'new' ClassOrInterfaceType DimExprs Dimsopt
\:$MakeArrayCreationUninitialized:\
/.$shared_function
//
// void MakeArrayCreationUninitialized();
//./

--1.1 feature
--
--ArrayCreationExpression ::= 'new' ArrayType ArrayInitializer
ArrayCreationInitialized ::= 'new' PrimitiveType Dims ArrayInitializer
\:$MakeArrayCreationInitialized:\
/.$location
void Parser::MakeArrayCreationInitialized()
{
    AstArrayCreationExpression* p = ast_pool -> NewArrayCreationExpression();
    p -> new_token = Token(1);
    p -> array_type = MakeArrayType(2);
    p -> array_initializer_opt = DYNAMIC_CAST<AstArrayInitializer*> (Sym(4));
    Sym(1) = p;
}
./

ArrayCreationInitialized ::= 'new' ClassOrInterfaceType Dims ArrayInitializer
\:$MakeArrayCreationInitialized:\
/.$shared_function
//
// void MakeArrayCreationInitialized();
//./

DimExprs ::= DimExpr
\:$StartList:\
/.$shared_StartList./

DimExprs ::= DimExprs DimExpr
\:$AddList2:\
/.$shared_AddList2./

DimExpr ::= '[' Expression ']'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstDimExpr* p = ast_pool -> NewDimExpr();
    p -> left_bracket_token = Token(1);
    p -> expression = DYNAMIC_CAST<AstExpression*> (Sym(2));
    p -> right_bracket_token = Token(3);
    Sym(1) = p;
}
./

Dims ::= '[' ']'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    Sym(1) = ast_pool -> NewBrackets(Token(1), Token(2));
}
./

Dims ::= Dims '[' ']'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstBrackets* p = DYNAMIC_CAST<AstBrackets*> (Sym(1));
    p -> right_bracket_token = Token(2);
    p -> dims++;
}
./

Dimsopt ::= $empty
\:$NullAction:\
/.$shared_NullAction./

Dimsopt ::= Dims
\:$NoAction:\
/.$shared_NoAction./

--
-- Added rule to make parsing 'super' '.' easier.
--
SuperAccess ::= 'super'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    Sym(1) = ast_pool -> NewSuperExpression(Token(1));
}
./

--
-- Added rule to make parsing 'super' '.' easier. Technically, only ClassType
-- is allowed instead of Name, but that would be ambiguous with qualified
-- names. The use of Marker allows us to share code.
--
--SuperAccess ::= ClassType '.' 'super'
SuperAccess ::= Name '.' Marker 'super'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstSuperExpression* p = ast_pool -> NewSuperExpression(Token(4));
    p -> base_opt = ast_pool -> NewTypeName(DYNAMIC_CAST<AstName*> (Sym(1)));
    Sym(1) = p;
}
./

--
-- The use of Marker allows us to share code.
--
--FieldAccess ::= Primary '.' 'Identifier'
FieldAccess ::= Primary '.' Marker 'Identifier'
\:$MakeFieldAccess:\
/.$location
void Parser::MakeFieldAccess()
{
    AstFieldAccess* p = ast_pool -> NewFieldAccess();
    p -> base = DYNAMIC_CAST<AstExpression*> (Sym(1));
    p -> identifier_token = Token(4);
    Sym(1) = p;
}
./

--
-- The use of Marker allows us to share code.  Likewise, SuperAccess
-- simplifies tree creation.
--
--FieldAccess ::= 'super' '.' 'Identifier'
--FieldAccess ::= ClassType '.' 'super' '.' 'Identifier'
FieldAccess ::= SuperAccess '.' Marker 'Identifier'
\:$MakeFieldAccess:\
/.$shared_function
//
// void MakeFieldAccess();
//./

--
-- Inline expand Name so we can distinguish the optional base from the required
-- method identifier.
--
--MethodInvocation ::= Name '(' ArgumentListopt ')'
MethodInvocation ::= 'Identifier' Arguments
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstMethodInvocation* p = ast_pool -> NewMethodInvocation(Token(1));
    p -> arguments = DYNAMIC_CAST<AstArguments*> (Sym(2));
    Sym(1) = p;
}
./

--
-- Inline expand Name so we can distinguish the optional base from the required
-- method identifier.  The use of Marker allows us to share code.
--
--MethodInvocation ::= Name '(' ArgumentListopt ')'
MethodInvocation ::= Name '.' Marker 'Identifier' Arguments
\:$MakeMethodInvocation:\
/.$location
void Parser::MakeMethodInvocation()
{
    AstMethodInvocation* p = ast_pool -> NewMethodInvocation(Token(4));
    p -> base_opt = DYNAMIC_CAST<AstExpression*> (Sym(1));
    p -> type_arguments_opt = MakeExplicitTypeArguments(3);
    p -> arguments = DYNAMIC_CAST<AstArguments*> (Sym(5));
    Sym(1) = p;
}
./

--
-- Parameterized types were added in JSR 14.
--
MethodInvocation ::= Name '.' TypeArguments 'Identifier' Arguments
\:$MakeMethodInvocation:\
/.$shared_function
//
// void MakeMethodInvocation();
//./

--
-- The use of Marker allows us to share code.
--
--MethodInvocation ::= Primary '.' 'Identifier' '(' ArgumentListopt ')'
MethodInvocation ::= Primary '.' Marker 'Identifier' Arguments
\:$MakeMethodInvocation:\
/.$shared_function
//
// void MakeMethodInvocation();
//./

--
-- Parameterized types were added in JSR 14.
--
MethodInvocation ::= Primary '.' TypeArguments 'Identifier' Arguments
\:$MakeMethodInvocation:\
/.$shared_function
//
// void MakeMethodInvocation();
//./

--
-- The use of Marker allows us to share code.  Likewise, SuperAccess
-- simplifies tree creation.
--
--MethodInvocation ::= 'super' '.' 'Identifier' '(' ArgumentListopt ')'
--MethodInvocation ::= ClassType '.' 'super' '.' 'Identifier' '('
--                     ArgumentListopt ')'
MethodInvocation ::= SuperAccess '.' Marker 'Identifier' Arguments
\:$MakeMethodInvocation:\
/.$shared_function
//
// void MakeMethodInvocation();
//./

--
-- Parameterized types were added in JSR 14.  Likewise, SuperAccess
-- simplifies tree creation.
--
--MethodInvocation ::= 'super' '.' TypeArguments 'Identifier' '('
--                     ArgumentListopt ')'
--MethodInvocation ::= ClassType '.' 'super' '.' TypeArguments 'Identifier' '('
--                     ArgumentListopt ')'
MethodInvocation ::= SuperAccess '.' TypeArguments 'Identifier' Arguments
\:$MakeMethodInvocation:\
/.$shared_function
//
// void MakeMethodInvocation();
//./

ArrayAccess ::= Name '[' Expression ']'
\:$MakeArrayAccess:\
/.$location
void Parser::MakeArrayAccess()
{
    AstArrayAccess* p = ast_pool -> NewArrayAccess();
    p -> base = DYNAMIC_CAST<AstExpression*> (Sym(1));
    p -> left_bracket_token = Token(2);
    p -> expression = DYNAMIC_CAST<AstExpression*> (Sym(3));
    p -> right_bracket_token = Token(4);
    Sym(1) = p;
}
./

ArrayAccess ::= PrimaryNoNewArray '[' Expression ']'
\:$MakeArrayAccess:\
/.$shared_function
//
// void MakeArrayAccess();
//./

--1.2 feature
--
-- Access of an initialized array is legal.  See above.
--
ArrayAccess ::= ArrayCreationInitialized '[' Expression ']'
\:$MakeArrayAccess:\
/.$shared_function
//
// void MakeArrayAccess();
//./

PostfixExpression ::= Primary
\:$NoAction:\
/.$shared_NoAction./

PostfixExpression ::= Name
\:$NoAction:\
/.$shared_NoAction./

PostfixExpression ::= PostIncrementExpression
\:$NoAction:\
/.$shared_NoAction./

PostfixExpression ::= PostDecrementExpression
\:$NoAction:\
/.$shared_NoAction./

PostfixExpressionNotName ::= Primary
\:$NoAction:\
/.$shared_NoAction./

PostfixExpressionNotName ::= PostIncrementExpression
\:$NoAction:\
/.$shared_NoAction./

PostfixExpressionNotName ::= PostDecrementExpression
\:$NoAction:\
/.$shared_NoAction./

PostIncrementExpression ::= PostfixExpression '++'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstPostUnaryExpression* p =
        ast_pool -> NewPostUnaryExpression(AstPostUnaryExpression::PLUSPLUS);
    p -> expression = DYNAMIC_CAST<AstExpression*> (Sym(1));
    p -> post_operator_token = Token(2);
    Sym(1) = p;
}
./

PostDecrementExpression ::= PostfixExpression '--'
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstPostUnaryExpression* p =
        ast_pool -> NewPostUnaryExpression(AstPostUnaryExpression::MINUSMINUS);
    p -> expression = DYNAMIC_CAST<AstExpression*> (Sym(1));
    p -> post_operator_token = Token(2);
    Sym(1) = p;
}
./

UnaryExpression ::= PreIncrementExpression
\:$NoAction:\
/.$shared_NoAction./

UnaryExpression ::= PreDecrementExpression
\:$NoAction:\
/.$shared_NoAction./

UnaryExpression ::= '+' UnaryExpression
\:$MakePreUnaryExpression:\
/.$location
void Parser::MakePreUnaryExpression()
{
    AstPreUnaryExpression::PreUnaryExpressionTag tag;
    switch (lex_stream -> Kind(Token(1)))
    {
    case TK_PLUS_PLUS: tag = AstPreUnaryExpression::PLUSPLUS; break;
    case TK_MINUS_MINUS: tag = AstPreUnaryExpression::MINUSMINUS; break;
    case TK_PLUS: tag = AstPreUnaryExpression::PLUS; break;
    case TK_MINUS: tag = AstPreUnaryExpression::MINUS; break;
    case TK_TWIDDLE: tag = AstPreUnaryExpression::TWIDDLE; break;
    case TK_NOT: tag = AstPreUnaryExpression::NOT; break;
    default: tag = AstPreUnaryExpression::NONE;
    }
    AstPreUnaryExpression* p = ast_pool -> NewPreUnaryExpression(tag);
    p -> pre_operator_token = Token(1);
    p -> expression = DYNAMIC_CAST<AstExpression*> (Sym(2));
    Sym(1) = p;
}
./

UnaryExpression ::= '-' UnaryExpression
\:$MakePreUnaryExpression:\
/.$shared_Unary./

UnaryExpression ::= UnaryExpressionNotPlusMinus
\:$NoAction:\
/.$shared_NoAction./

UnaryExpressionNotName ::= PreIncrementExpression
\:$NoAction:\
/.$shared_NoAction./

UnaryExpressionNotName ::= PreDecrementExpression
\:$NoAction:\
/.$shared_NoAction./

UnaryExpressionNotName ::= '+' UnaryExpression
\:$MakePreUnaryExpression:\
/.$shared_Unary./

UnaryExpressionNotName ::= '-' UnaryExpression
\:$MakePreUnaryExpression:\
/.$shared_Unary./

UnaryExpressionNotName ::= UnaryExpressionNotPlusMinusNotName
\:$NoAction:\
/.$shared_NoAction./

PreIncrementExpression ::= '++' UnaryExpression
\:$MakePreUnaryExpression:\
/.$shared_Unary./

PreDecrementExpression ::= '--' UnaryExpression
\:$MakePreUnaryExpression:\
/.$shared_Unary./

UnaryExpressionNotPlusMinus ::= PostfixExpression
\:$NoAction:\
/.$shared_NoAction./

UnaryExpressionNotPlusMinus ::= '~' UnaryExpression
\:$MakePreUnaryExpression:\
/.$shared_Unary./

UnaryExpressionNotPlusMinus ::= '!' UnaryExpression
\:$MakePreUnaryExpression:\
/.$shared_Unary./

UnaryExpressionNotPlusMinus ::= CastExpression
\:$NoAction:\
/.$shared_NoAction./

UnaryExpressionNotPlusMinusNotName ::= PostfixExpressionNotName
\:$NoAction:\
/.$shared_NoAction./

UnaryExpressionNotPlusMinusNotName ::= '~' UnaryExpression
\:$MakePreUnaryExpression:\
/.$shared_Unary./

UnaryExpressionNotPlusMinusNotName ::= '!' UnaryExpression
\:$MakePreUnaryExpression:\
/.$shared_Unary./

UnaryExpressionNotPlusMinusNotName ::= CastExpression
\:$NoAction:\
/.$shared_NoAction./

--
-- Due to grammar ambiguities, we must rewrite this (otherwise, it is not
-- obvious whether "(a<b" starts a parenthesized expression or a cast). Note
-- that our rewrite guarantees that the contents of the parenthesis will
-- syntactically be a type, based on the way we factored parenthesized
-- expressions in Primary.
--
-- JLS2 15 lists:
--CastExpression ::= '(' PrimitiveType ')' UnaryExpression
--CastExpression ::= '(' ReferenceType ')' UnaryExpressionNotPlusMinus
-- JLS1 suggests:
--CastExpression ::= '(' PrimitiveType Dimsopt ')' UnaryExpression
--CastExpression ::= '(' Expression ')' UnaryExpressionNotPlusMinus
--CastExpression ::= '(' Name Dims ')' UnaryExpressionNotPlusMinus
--
CastExpression ::= '(' PrimitiveType Dimsopt ')' UnaryExpression
\:$MakeCastExpression:\
/.$location
void Parser::MakeCastExpression() { MakeCastExpression(MakeArrayType(2), 4); }

//
// Builds a cast expression. type must be the target AstType, and tokennum
// should point to the ')'.
//
void Parser::MakeCastExpression(AstType* type, int tokennum)
{
    AstCastExpression* p = ast_pool -> NewCastExpression();
    p -> left_parenthesis_token = Token(1);
    p -> type = type;
    p -> right_parenthesis_token = Token(tokennum);
    p -> expression = DYNAMIC_CAST<AstExpression*> (Sym(tokennum + 1));
    Sym(1) = p;
}
./

--
-- The use of Marker allows us to share code.
--
CastExpression ::= '(' Name Marker ')' UnaryExpressionNotPlusMinus
\:$MakeCastExpression:\
/.$shared_function
//
// void MakeCastExpression();
//./

CastExpression ::= '(' Name Dims ')' UnaryExpressionNotPlusMinus
\:$MakeCastExpression:\
/.$shared_function
//
// void MakeCastExpression();
//./

--
-- Parameterized types were added in JSR 14.
--
CastExpression ::= '(' Name '<' TypeArgumentList1 Dimsopt ')'
                   UnaryExpressionNotPlusMinus
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    Sym(4) = MakeTypeArguments(2);
    MakeCastExpression(MakeArrayType(4), 6);
}
./

--
-- Parameterized types were added in JSR 14.
--
CastExpression ::= '(' Name '<' TypeArgumentList1 '.' ClassOrInterfaceType
                   Dimsopt ')' UnaryExpressionNotPlusMinus
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstTypeName* p = DYNAMIC_CAST<AstTypeName*> (Sym(6));
    while (p -> base_opt)
        p = p -> base_opt;
    p -> base_opt = MakeTypeArguments(2);
    MakeCastExpression(MakeArrayType(6), 8);
}
./

MultiplicativeExpression ::= UnaryExpression
\:$NoAction:\
/.$shared_NoAction./

MultiplicativeExpression ::= MultiplicativeExpression '*' UnaryExpression
\:$MakeBinaryExpression:\
/.$location
//
// This creates a binary expression of the named type.
//
void Parser::MakeBinaryExpression()
{
    AstBinaryExpression::BinaryExpressionTag tag;
    switch (lex_stream -> Kind(Token(2)))
    {
    case TK_MULTIPLY: tag = AstBinaryExpression::STAR; break;
    case TK_DIVIDE: tag = AstBinaryExpression::SLASH; break;
    case TK_REMAINDER: tag = AstBinaryExpression::MOD; break;
    case TK_PLUS: tag = AstBinaryExpression::PLUS; break;
    case TK_MINUS: tag = AstBinaryExpression::MINUS; break;
    case TK_LEFT_SHIFT: tag = AstBinaryExpression::LEFT_SHIFT; break;
    case TK_RIGHT_SHIFT: tag = AstBinaryExpression::RIGHT_SHIFT; break;
    case TK_UNSIGNED_RIGHT_SHIFT:
        tag = AstBinaryExpression::UNSIGNED_RIGHT_SHIFT; break;
    case TK_LESS: tag = AstBinaryExpression::LESS; break;
    case TK_GREATER: tag = AstBinaryExpression::GREATER; break;
    case TK_LESS_EQUAL: tag = AstBinaryExpression::LESS_EQUAL; break;
    case TK_GREATER_EQUAL: tag = AstBinaryExpression::GREATER_EQUAL; break;
    case TK_EQUAL_EQUAL: tag = AstBinaryExpression::EQUAL_EQUAL; break;
    case TK_NOT_EQUAL: tag = AstBinaryExpression::NOT_EQUAL; break;
    case TK_AND: tag = AstBinaryExpression::AND; break;
    case TK_XOR: tag = AstBinaryExpression::XOR; break;
    case TK_OR: tag = AstBinaryExpression::IOR; break;
    case TK_AND_AND: tag = AstBinaryExpression::AND_AND; break;
    case TK_OR_OR: tag = AstBinaryExpression::OR_OR; break;
    default: tag = AstBinaryExpression::NONE;
    }
    AstBinaryExpression* p = ast_pool -> NewBinaryExpression(tag);
    p -> left_expression = DYNAMIC_CAST<AstExpression*> (Sym(1));
    p -> binary_operator_token = Token(2);
    p -> right_expression = DYNAMIC_CAST<AstExpression*> (Sym(3));
    Sym(1) = p;
}
./

MultiplicativeExpression ::= MultiplicativeExpression '/' UnaryExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

MultiplicativeExpression ::= MultiplicativeExpression '%' UnaryExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

MultiplicativeExpressionNotName ::= UnaryExpressionNotName
\:$NoAction:\
/.$shared_NoAction./

MultiplicativeExpressionNotName ::= MultiplicativeExpressionNotName '*'
                                    UnaryExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

MultiplicativeExpressionNotName ::= Name '*' UnaryExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

MultiplicativeExpressionNotName ::= MultiplicativeExpressionNotName '/'
                                    UnaryExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

MultiplicativeExpressionNotName ::= Name '/' UnaryExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

MultiplicativeExpressionNotName ::= MultiplicativeExpressionNotName '%'
                                    UnaryExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

MultiplicativeExpressionNotName ::= Name '%' UnaryExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

AdditiveExpression ::= MultiplicativeExpression
\:$NoAction:\
/.$shared_NoAction./

AdditiveExpression ::= AdditiveExpression '+' MultiplicativeExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

AdditiveExpression ::= AdditiveExpression '-' MultiplicativeExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

AdditiveExpressionNotName ::= MultiplicativeExpressionNotName
\:$NoAction:\
/.$shared_NoAction./

AdditiveExpressionNotName ::= AdditiveExpressionNotName '+'
                              MultiplicativeExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

AdditiveExpressionNotName ::= Name '+' MultiplicativeExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

AdditiveExpressionNotName ::= AdditiveExpressionNotName '-'
                              MultiplicativeExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

AdditiveExpressionNotName ::= Name '-' MultiplicativeExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

ShiftExpression ::= AdditiveExpression
\:$NoAction:\
/.$shared_NoAction./

ShiftExpression ::= ShiftExpression '<<' AdditiveExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

ShiftExpression ::= ShiftExpression '>>' AdditiveExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

ShiftExpression ::= ShiftExpression '>>>' AdditiveExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

ShiftExpressionNotName ::= AdditiveExpressionNotName
\:$NoAction:\
/.$shared_NoAction./

ShiftExpressionNotName ::= ShiftExpressionNotName '<<' AdditiveExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

ShiftExpressionNotName ::= Name '<<' AdditiveExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

ShiftExpressionNotName ::= ShiftExpressionNotName '>>' AdditiveExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

ShiftExpressionNotName ::= Name '>>' AdditiveExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

ShiftExpressionNotName ::= ShiftExpressionNotName '>>>' AdditiveExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

ShiftExpressionNotName ::= Name '>>>' AdditiveExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

RelationalExpression ::= ShiftExpression
\:$NoAction:\
/.$shared_NoAction./

--
-- Relational expressions do not operate on boolean. Rewriting this
-- rule avoids an ambiguity in generics with no semantic penalty. The
-- alternative is to lower the precedence of instanceof.
--
--RelationalExpression ::= RelationalExpression '<' ShiftExpression
RelationalExpression ::= ShiftExpression '<' ShiftExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

RelationalExpression ::= RelationalExpression '>' ShiftExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

RelationalExpression ::= RelationalExpression '<=' ShiftExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

RelationalExpression ::= RelationalExpression '>=' ShiftExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

RelationalExpression ::= RelationalExpression 'instanceof' ReferenceType
\:$MakeInstanceofExpression:\
/.$location
void Parser::MakeInstanceofExpression()
{
    AstInstanceofExpression* p = ast_pool -> NewInstanceofExpression();
    p -> expression = DYNAMIC_CAST<AstExpression*> (Sym(1));
    p -> instanceof_token = Token(2);
    p -> type = DYNAMIC_CAST<AstType*> (Sym(3));
    Sym(1) = p;
}
./

RelationalExpressionNotName ::= ShiftExpressionNotName
\:$NoAction:\
/.$shared_NoAction./

--RelationalExpressionNotName ::= RelationalExpressionNotName '<'
--                                ShiftExpression
RelationalExpressionNotName ::= ShiftExpressionNotName '<' ShiftExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

RelationalExpressionNotName ::= Name '<' ShiftExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

--RelationalExpressionNotName ::= RelationalExpressionNotName '>'
--                                ShiftExpression
RelationalExpressionNotName ::= ShiftExpressionNotName '>' ShiftExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

RelationalExpressionNotName ::= Name '>' ShiftExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

RelationalExpressionNotName ::= RelationalExpressionNotName '<='
                                ShiftExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

RelationalExpressionNotName ::= Name '<=' ShiftExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

RelationalExpressionNotName ::= RelationalExpressionNotName '>='
                                ShiftExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

RelationalExpressionNotName ::= Name '>=' ShiftExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

RelationalExpressionNotName ::= RelationalExpressionNotName 'instanceof'
                                ReferenceType
\:$MakeInstanceofExpression:\
/.$shared_function
//
// void MakeInstanceofExpression();
//./

RelationalExpressionNotName ::= Name 'instanceof' ReferenceType
\:$MakeInstanceofExpression:\
/.$shared_function
//
// void MakeInstanceofExpression();
//./

EqualityExpression ::= RelationalExpression
\:$NoAction:\
/.$shared_NoAction./

EqualityExpression ::= EqualityExpression '==' RelationalExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

EqualityExpression ::= EqualityExpression '!=' RelationalExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

EqualityExpressionNotName ::= RelationalExpressionNotName
\:$NoAction:\
/.$shared_NoAction./

EqualityExpressionNotName ::= EqualityExpressionNotName '=='
                              RelationalExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

EqualityExpressionNotName ::= Name '==' RelationalExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

EqualityExpressionNotName ::= EqualityExpressionNotName '!='
                              RelationalExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

EqualityExpressionNotName ::= Name '!=' RelationalExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

AndExpression ::= EqualityExpression
\:$NoAction:\
/.$shared_NoAction./

AndExpression ::= AndExpression '&' EqualityExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

AndExpressionNotName ::= EqualityExpressionNotName
\:$NoAction:\
/.$shared_NoAction./

AndExpressionNotName ::= AndExpressionNotName '&' EqualityExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

AndExpressionNotName ::= Name '&' EqualityExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

ExclusiveOrExpression ::= AndExpression
\:$NoAction:\
/.$shared_NoAction./

ExclusiveOrExpression ::= ExclusiveOrExpression '^' AndExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

ExclusiveOrExpressionNotName ::= AndExpressionNotName
\:$NoAction:\
/.$shared_NoAction./

ExclusiveOrExpressionNotName ::= ExclusiveOrExpressionNotName '^' AndExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

ExclusiveOrExpressionNotName ::= Name '^' AndExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

InclusiveOrExpression ::= ExclusiveOrExpression
\:$NoAction:\
/.$shared_NoAction./

InclusiveOrExpression ::= InclusiveOrExpression '|' ExclusiveOrExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

InclusiveOrExpressionNotName ::= ExclusiveOrExpressionNotName
\:$NoAction:\
/.$shared_NoAction./

InclusiveOrExpressionNotName ::= InclusiveOrExpressionNotName '|'
                                 ExclusiveOrExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

InclusiveOrExpressionNotName ::= Name '|' ExclusiveOrExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

ConditionalAndExpression ::= InclusiveOrExpression
\:$NoAction:\
/.$shared_NoAction./

ConditionalAndExpression ::= ConditionalAndExpression '&&'
                             InclusiveOrExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

ConditionalAndExpressionNotName ::= InclusiveOrExpressionNotName
\:$NoAction:\
/.$shared_NoAction./

ConditionalAndExpressionNotName ::= ConditionalAndExpressionNotName '&&'
                                    InclusiveOrExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

ConditionalAndExpressionNotName ::= Name '&&' InclusiveOrExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

ConditionalOrExpression ::= ConditionalAndExpression
\:$NoAction:\
/.$shared_NoAction./

ConditionalOrExpression ::= ConditionalOrExpression '||'
                            ConditionalAndExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

ConditionalOrExpressionNotName ::= ConditionalAndExpressionNotName
\:$NoAction:\
/.$shared_NoAction./

ConditionalOrExpressionNotName ::= ConditionalOrExpressionNotName '||'
                                   ConditionalAndExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

ConditionalOrExpressionNotName ::= Name '||' ConditionalAndExpression
\:$MakeBinaryExpression:\
/.$shared_Binary./

ConditionalExpression ::= ConditionalOrExpression
\:$NoAction:\
/.$shared_NoAction./

ConditionalExpression ::= ConditionalOrExpression '?' Expression ':'
                          ConditionalExpression
\:$MakeConditionalExpression:\
/.$location
void Parser::MakeConditionalExpression()
{
    AstConditionalExpression* p = ast_pool -> NewConditionalExpression();
    p -> test_expression = DYNAMIC_CAST<AstExpression*> (Sym(1));
    p -> question_token = Token(2);
    p -> true_expression = DYNAMIC_CAST<AstExpression*> (Sym(3));
    p -> colon_token = Token(4);
    p -> false_expression = DYNAMIC_CAST<AstExpression*> (Sym(5));
    Sym(1) = p;
}
./

ConditionalExpressionNotName ::= ConditionalOrExpressionNotName
\:$NoAction:\
/.$shared_NoAction./

ConditionalExpressionNotName ::= ConditionalOrExpressionNotName '?' Expression
                                 ':' ConditionalExpression
\:$MakeConditionalExpression:\
/.$shared_function
//
// void MakeConditionalExpression();
//./

ConditionalExpressionNotName ::= Name '?' Expression ':' ConditionalExpression
\:$MakeConditionalExpression:\
/.$shared_function
//
// void MakeConditionalExpression();
//./

AssignmentExpression ::= ConditionalExpression
\:$NoAction:\
/.$shared_NoAction./

AssignmentExpression ::= Assignment
\:$NoAction:\
/.$shared_NoAction./

AssignmentExpressionNotName ::= ConditionalExpressionNotName
\:$NoAction:\
/.$shared_NoAction./

AssignmentExpressionNotName ::= Assignment
\:$NoAction:\
/.$shared_NoAction./

--
-- The original grammar uses LeftHandSide, instead of PostfixExpression.
-- However, parenthesized variables were added in JLS 2, and the
-- grammar is ambiguous unless we include all non-assignment
-- expressions. The semantic pass will filter out bad left-hand sides.
--
--Assignment ::= LeftHandSide AssignmentOperator AssignmentExpression
Assignment ::= PostfixExpression AssignmentOperator AssignmentExpression
\:$action:\
/.$location
void Parser::Act$rule_number()
{
    AstAssignmentExpression::AssignmentExpressionTag tag;
    switch (lex_stream -> Kind(Token(2)))
    {
    case TK_EQUAL: tag = AstAssignmentExpression::SIMPLE_EQUAL; break;
    case TK_MULTIPLY_EQUAL: tag = AstAssignmentExpression::STAR_EQUAL; break;
    case TK_DIVIDE_EQUAL: tag = AstAssignmentExpression::SLASH_EQUAL; break;
    case TK_REMAINDER_EQUAL: tag = AstAssignmentExpression::MOD_EQUAL; break;
    case TK_PLUS_EQUAL: tag = AstAssignmentExpression::PLUS_EQUAL; break;
    case TK_MINUS_EQUAL: tag = AstAssignmentExpression::MINUS_EQUAL; break;
    case TK_LEFT_SHIFT_EQUAL:
        tag = AstAssignmentExpression::LEFT_SHIFT_EQUAL; break;
    case TK_RIGHT_SHIFT_EQUAL:
        tag = AstAssignmentExpression::RIGHT_SHIFT_EQUAL; break;
    case TK_UNSIGNED_RIGHT_SHIFT_EQUAL:
        tag = AstAssignmentExpression::UNSIGNED_RIGHT_SHIFT_EQUAL; break;
    case TK_AND_EQUAL: tag = AstAssignmentExpression::AND_EQUAL; break;
    case TK_XOR_EQUAL: tag = AstAssignmentExpression::XOR_EQUAL; break;
    case TK_OR_EQUAL: tag = AstAssignmentExpression::IOR_EQUAL; break;
    default: tag = AstAssignmentExpression::NONE;
    }
    AstAssignmentExpression* p =
        ast_pool -> NewAssignmentExpression(tag, Token(2));
    p -> left_hand_side = DYNAMIC_CAST<AstExpression*> (Sym(1));
    p -> expression = DYNAMIC_CAST<AstExpression*> (Sym(3));
    Sym(1) = p;
}
./

--
-- See comments above for Assignment - LeftHandSide is now a useless rule.
--
--LeftHandSide -> Name
--LeftHandSide -> FieldAccess
--LeftHandSide -> ArrayAccess

AssignmentOperator ::= '='
\:$NoAction:\
/.$shared_NoAction./

AssignmentOperator ::= '*='
\:$NoAction:\
/.$shared_NoAction./

AssignmentOperator ::= '/='
\:$NoAction:\
/.$shared_NoAction./

AssignmentOperator ::= '%='
\:$NoAction:\
/.$shared_NoAction./

AssignmentOperator ::= '+='
\:$NoAction:\
/.$shared_NoAction./

AssignmentOperator ::= '-='
\:$NoAction:\
/.$shared_NoAction./

AssignmentOperator ::= '<<='
\:$NoAction:\
/.$shared_NoAction./

AssignmentOperator ::= '>>='
\:$NoAction:\
/.$shared_NoAction./

AssignmentOperator ::= '>>>='
\:$NoAction:\
/.$shared_NoAction./

AssignmentOperator ::= '&='
\:$NoAction:\
/.$shared_NoAction./

AssignmentOperator ::= '^='
\:$NoAction:\
/.$shared_NoAction./

AssignmentOperator ::= '|='
\:$NoAction:\
/.$shared_NoAction./

Expression ::= AssignmentExpression
\:$NoAction:\
/.$shared_NoAction./

Expressionopt ::= $empty
\:$NullAction:\
/.$shared_NullAction./

Expressionopt ::= Expression
\:$NoAction:\
/.$shared_NoAction./

ExpressionNotName ::= AssignmentExpressionNotName
\:$NoAction:\
/.$shared_NoAction./

--
-- Simplify the syntax tree.
--
--ConstantExpression ::= Expression

-----------------------------------------------------------------------------
--
-- The following rules are for optional and helper productions.
--
-----------------------------------------------------------------------------

--
-- This rule exists solely to put NULL on the symbol stack, allowing us to
-- share productions that differ by the presence or absence of a rule.
--
Marker ::= $empty
\:$NullAction:\
/.$shared_NullAction./

,opt ::= $empty
\:$NoAction:\
/.$shared_NoAction./

,opt ::= ','
\:$NoAction:\
/.$shared_NoAction./

Identifieropt ::= $empty
\:$NoAction:\
/.$shared_NoAction./

Identifieropt ::= 'Identifier'
\:$NoAction:\
/.$shared_NoAction./

PackageHeaderMarker ::= $empty
\:$action:\
/.$location
//
// When this function is invoked, if the "parse_package_header_only" flag
// is turned on, we skip to the end-of-file token.
//
void Parser::Act$rule_number()
{
    if (parse_package_header_only)
        // point to the EOF token
        lex_stream -> Reset(lex_stream -> NumTokens() - 1);
    Sym(1) = NULL;
}
./

MethodHeaderMarker ::= $empty
\:$action:\
/.$location
//
// When this function is invoked, if the "parse_header_only" flag
// is turned on, the body of the method being parsed is skipped.
//
void Parser::Act$rule_number()
{
    if (parse_header_only)
    {
        TokenObject token = Token(1);

        //
        // If the first token immediately following the method header
        // is not an open brace, then we have a syntactic error. Do
        // nothing and let the error recovery take care of it.
        //
        if (lex_stream -> Kind(token) == TK_LBRACE)
            lex_stream -> Reset(lex_stream -> MatchingBrace(token));
    }
    Sym(1) = NULL;
}
./

-----------------------------------------------------------------------------
--
-- These rules add generics. Also search for JSR 14 in the comments above.
--
-----------------------------------------------------------------------------

--
-- Parameterized types were added in JSR 14.
--
TypeArguments ::= '<' TypeArgumentList1
\:$SetSym1ToSym2:\
/.$shared_function
//
// void SetSym1ToSym2();
//./

--
-- Parameterized types were added in JSR 14.
--
TypeArgumentsopt ::= $empty
\:$NullAction:\
/.$shared_NullAction./

--
-- Parameterized types were added in JSR 14.
--
TypeArgumentsopt ::= TypeArguments
\:$NoAction:\
/.$shared_NoAction./

--
-- Parameterized types were added in JSR 14. We inline expanded WildcardBounds
-- for simplicity. The use of Marker allows us to share code.
--
--Wildcard ::= '?' WildcardBoundsopt
--WildcardBounds ::= extends ReferenceType
--WildcardBounds ::= super ReferenceType
--
Wildcard ::= '?' Marker Marker Marker
\:$MakeWildcard:\
/.$location
void Parser::MakeWildcard()
{
    AstWildcard* p = ast_pool -> NewWildcard(Token(1));
    if (Token(3) > Token(2))
    {
        p -> extends_token_opt = Token(2);
        p -> bounds_opt = DYNAMIC_CAST<AstType*> (Sym(4));
    }
    else if (Token(4) > Token(3))
    {
        p -> super_token_opt = Token(3);
        p -> bounds_opt = DYNAMIC_CAST<AstType*> (Sym(4));
    }
    Sym(1) = p;
}
./

--
-- Parameterized types were added in JSR 14.
-- The use of Marker allows us to share code.
--
Wildcard ::= '?' 'extends' Marker ReferenceType
\:$MakeWildcard:\
/.$shared_function
//
// void MakeWildcard();
//./

--
-- Parameterized types were added in JSR 14.
-- The use of Marker allows us to share code.
--
Wildcard ::= '?' Marker 'super' ReferenceType
\:$MakeWildcard:\
/.$shared_function
//
// void MakeWildcard();
//./

--
-- Parameterized types were added in JSR 14.
-- The use of Marker allows us to share code.
--
Wildcard1 ::= '?' Marker Marker '>'
\:$MakeWildcard:\
/.$shared_function
//
// void MakeWildcard();
//./

--
-- Parameterized types were added in JSR 14.
-- The use of Marker allows us to share code.
--
Wildcard1 ::= '?' 'extends' Marker ReferenceType1
\:$MakeWildcard:\
/.$shared_function
//
// void MakeWildcard();
//./

--
-- Parameterized types were added in JSR 14.
-- The use of Marker allows us to share code.
--
Wildcard1 ::= '?' Marker 'super' ReferenceType1
\:$MakeWildcard:\
/.$shared_function
//
// void MakeWildcard();
//./

--
-- Parameterized types were added in JSR 14.
-- The use of Marker allows us to share code.
--
Wildcard2 ::= '?' Marker Marker '>>'
\:$MakeWildcard:\
/.$shared_function
//
// void MakeWildcard();
//./

--
-- Parameterized types were added in JSR 14.
-- The use of Marker allows us to share code.
--
Wildcard2 ::= '?' 'extends' Marker ReferenceType2
\:$MakeWildcard:\
/.$shared_function
//
// void MakeWildcard();
//./

--
-- Parameterized types were added in JSR 14.
-- The use of Marker allows us to share code.
--
Wildcard2 ::= '?' Marker 'super' ReferenceType2
\:$MakeWildcard:\
/.$shared_function
//
// void MakeWildcard();
//./

--
-- Parameterized types were added in JSR 14.
-- The use of Marker allows us to share code.
--
Wildcard3 ::= '?' Marker Marker '>>>'
\:$MakeWildcard:\
/.$shared_function
//
// void MakeWildcard();
//./

--
-- Parameterized types were added in JSR 14.
-- The use of Marker allows us to share code.
--
Wildcard3 ::= '?' 'extends' Marker ReferenceType3
\:$MakeWildcard:\
/.$shared_function
//
// void MakeWildcard();
//./

--
-- Parameterized types were added in JSR 14.
-- The use of Marker allows us to share code.
--
Wildcard3 ::= '?' Marker 'super' ReferenceType3
\:$MakeWildcard:\
/.$shared_function
//
// void MakeWildcard();
//./

--
-- Parameterized types were added in JSR 14.
--
TypeArgumentList ::= TypeArgument
\:$StartList:\
/.$shared_StartList./

--
-- Parameterized types were added in JSR 14.
--
TypeArgumentList ::= TypeArgumentList ',' TypeArgument
\:$AddList3:\
/.$shared_AddList3./

--
-- Parameterized types were added in JSR 14.
--
TypeArgumentList1 ::= TypeArgument1
\:$StartList:\
/.$shared_StartList./

--
-- Parameterized types were added in JSR 14.
--
TypeArgumentList1 ::= TypeArgumentList ',' TypeArgument1
\:$AddList3:\
/.$shared_AddList3./

--
-- Parameterized types were added in JSR 14.
--
TypeArgumentList2 ::= TypeArgument2
\:$StartList:\
/.$shared_StartList./

--
-- Parameterized types were added in JSR 14.
--
TypeArgumentList2 ::= TypeArgumentList ',' TypeArgument2
\:$AddList3:\
/.$shared_AddList3./

--
-- Parameterized types were added in JSR 14.
--
TypeArgumentList3 ::= TypeArgument3
\:$StartList:\
/.$shared_StartList./

--
-- Parameterized types were added in JSR 14.
--
TypeArgumentList3 ::= TypeArgumentList ',' TypeArgument3
\:$AddList3:\
/.$shared_AddList3./

--
-- Parameterized types were added in JSR 14.
--
TypeArgument ::= ReferenceType
\:$NoAction:\
/.$shared_NoAction./

--
-- Parameterized types were added in JSR 14.
--
TypeArgument ::= Wildcard
\:$NoAction:\
/.$shared_NoAction./

--
-- Parameterized types were added in JSR 14.
--
TypeArgument1 ::= ReferenceType1
\:$NoAction:\
/.$shared_NoAction./

--
-- Parameterized types were added in JSR 14.
--
TypeArgument1 ::= Wildcard1
\:$NoAction:\
/.$shared_NoAction./

--
-- Parameterized types were added in JSR 14.
--
TypeArgument2 ::= ReferenceType2
\:$NoAction:\
/.$shared_NoAction./

--
-- Parameterized types were added in JSR 14.
--
TypeArgument2 ::= Wildcard2
\:$NoAction:\
/.$shared_NoAction./

--
-- Parameterized types were added in JSR 14.
--
TypeArgument3 ::= ReferenceType3
\:$NoAction:\
/.$shared_NoAction./

--
-- Parameterized types were added in JSR 14.
--
TypeArgument3 ::= Wildcard3
\:$NoAction:\
/.$shared_NoAction./

--
-- Parameterized types were added in JSR 14.
--
ReferenceType1 ::= ReferenceType '>'
\:$NoAction:\
/.$shared_NoAction./

--
-- Parameterized types were added in JSR 14.
-- Use of Marker allows us to easily find the closing '>>'.
--
ReferenceType1 ::= ClassOrInterface '<' TypeArgumentList2 Marker
\:$MakeTypeArguments:\
/.$shared_function
//
// void MakeTypeArguments();
//./

--
-- Parameterized types were added in JSR 14.
--
ReferenceType2 ::= ReferenceType '>>'
\:$NoAction:\
/.$shared_NoAction./

--
-- Parameterized types were added in JSR 14.
-- Use of Marker allows us to easily find the closing '>>>'.
--
ReferenceType2 ::= ClassOrInterface '<' TypeArgumentList3 Marker
\:$MakeTypeArguments:\
/.$shared_function
//
// void MakeTypeArguments();
//./

--
-- Parameterized types were added in JSR 14.
--
ReferenceType3 ::= ReferenceType '>>>'
\:$NoAction:\
/.$shared_NoAction./

--
-- Parameterized types were added in JSR 14.
--
TypeParameters ::= '<' TypeParameterList1
\:$SetSym1ToSym2:\
/.$shared_function
//
// void SetSym1ToSym2();
//./

--
-- Parameterized types were added in JSR 14.
--
TypeParametersopt ::= $empty
\:$NullAction:\
/.$shared_NullAction./

--
-- Parameterized types were added in JSR 14.
--
TypeParametersopt ::= TypeParameters
\:$NoAction:\
/.$shared_NoAction./

--
-- Parameterized types were added in JSR 14.
--
TypeParameterList ::= TypeParameter
\:$StartList:\
/.$shared_StartList./

--
-- Parameterized types were added in JSR 14.
--
TypeParameterList ::= TypeParameterList ',' TypeParameter
\:$AddList3:\
/.$shared_AddList3./

--
-- Parameterized types were added in JSR 14.
--
TypeParameterList1 ::= TypeParameter1
\:$StartList:\
/.$shared_StartList./

--
-- Parameterized types were added in JSR 14.
--
TypeParameterList1 ::= TypeParameterList ',' TypeParameter1
\:$AddList3:\
/.$shared_AddList3./

--
-- Parameterized types were added in JSR 14.
--
TypeParameter ::= 'Identifier' TypeBoundopt
\:$MakeTypeParameter:\
/.$location
void Parser::MakeTypeParameter()
{
    AstTypeParameter* p = ast_pool -> NewTypeParameter(Token(1));
    if (Sym(2))
    {
        //
        // Remember, we built the circular list with the first element at
        // the tail of the list, because of the grammar of TypeBound.
        //
        AstListNode* tail = DYNAMIC_CAST<AstListNode*> (Sym(2));
        p -> AllocateBounds(tail -> index + 1);
        AstListNode* root = tail;
        do
        {
            p -> AddBound(DYNAMIC_CAST<AstTypeName*> (root -> element));
            root = root -> next;
        } while (root != tail);
        FreeCircularList(tail);
    }
    Sym(1) = p;
}
./

--
-- Parameterized types were added in JSR 14.
-- The use of Marker allows us to share code.
--
TypeParameter1 ::= 'Identifier' Marker '>'
\:$MakeTypeParameter:\
/.$shared_function
//
// void MakeTypeParameter();
//./

--
-- Parameterized types were added in JSR 14.
--
TypeParameter1 ::= 'Identifier' TypeBound1
\:$MakeTypeParameter:\
/.$shared_function
//
// void MakeTypeParameter();
//./

--
-- Parameterized types were added in JSR 14.
--
TypeBound ::= 'extends' ReferenceType AdditionalBoundListopt
\:$MakeTypeBound:\
/.$location
void Parser::MakeTypeBound()
{
    //
    // Unlike most AstListNodes, we stick Sym(2) at the end of the list
    // instead of the beginning. MakeTypeParameter expects this unusual
    // ordering, because it is easier than rewriting the grammar to build the
    // list in lexical order.
    //
    if (Sym(3))
    {
        Sym(1) = Sym(3);
        AddList2();
    }
    else
    {
        Sym(1) = Sym(2);
        StartList();
    }
}
./

--
-- Parameterized types were added in JSR 14.
--
TypeBoundopt ::= $empty
\:$NullAction:\
/.$shared_NullAction./

--
-- Parameterized types were added in JSR 14.
--
TypeBoundopt ::= TypeBound
\:$NoAction:\
/.$shared_NoAction./

--
-- Parameterized types were added in JSR 14.
-- The use of Marker allows us to share code.
--
TypeBound1 ::= 'extends' ReferenceType1 Marker
\:$MakeTypeBound:\
/.$shared_function
//
// void MakeTypeBound();
//./

--
-- Parameterized types were added in JSR 14.
--
TypeBound1 ::= 'extends' ReferenceType AdditionalBoundList1
\:$MakeTypeBound:\
/.$shared_function
//
// void MakeTypeBound();
//./

--
-- Parameterized types were added in JSR 14.
--
AdditionalBoundList ::= AdditionalBound
\:$StartList:\
/.$shared_StartList./

--
-- Parameterized types were added in JSR 14.
--
AdditionalBoundList ::= AdditionalBoundList AdditionalBound
\:$AddList2:\
/.$shared_AddList2./

--
-- Parameterized types were added in JSR 14.
--
AdditionalBoundListopt ::= $empty
\:$NullAction:\
/.$shared_NullAction./

--
-- Parameterized types were added in JSR 14.
--
AdditionalBoundListopt ::= AdditionalBoundList
\:$NoAction:\
/.$shared_NoAction./

--
-- Parameterized types were added in JSR 14.
--
AdditionalBoundList1 ::= AdditionalBound1
\:$StartList:\
/.$shared_StartList./

--
-- Parameterized types were added in JSR 14.
--
AdditionalBoundList1 ::= AdditionalBoundList AdditionalBound1
\:$AddList2:\
/.$shared_AddList2./

--
-- Parameterized types were added in JSR 14.
--
AdditionalBound ::= '&' ClassOrInterfaceType
\:$SetSym1ToSym2:\
/.$shared_function
//
// void SetSym1ToSym2();
//./

--
-- Parameterized types were added in JSR 14.
--
AdditionalBound1 ::= '&' ClassOrInterfaceType1
\:$SetSym1ToSym2:\
/.$shared_function
//
// void SetSym1ToSym2();
//./

--
-- Parameterized types were added in JSR 14.
--
ClassOrInterfaceType1 ::= ClassOrInterfaceType '>'
\:$NoAction:\
/.$shared_NoAction./

--
-- Parameterized types were added in JSR 14.
-- Use of Marker allows us to easily find the closing '>>'.
--
ClassOrInterfaceType1 ::= ClassOrInterface '<' TypeArgumentList2 Marker
\:$MakeTypeArguments:\
/.$shared_function
//
// void MakeTypeArguments();
//./

------ Finish off the files
/.
#ifdef HAVE_JIKES_NAMESPACE
} // Close namespace Jikes block
#endif
./

\:
#ifndef HEADERS
    return;
}

#ifdef HAVE_JIKES_NAMESPACE
} // Close namespace Jikes block
#endif

#endif // ! HEADERS
:\

-- Names allow diagnose.cpp debug output to be more legible
$names

BodyMarker ::= '"class Identifier { ... MethodHeader "'

PLUS_PLUS ::= '++'
MINUS_MINUS ::= '--'
EQUAL_EQUAL ::= '=='
LESS_EQUAL ::= '<='
GREATER_EQUAL ::= '>='
NOT_EQUAL ::= '!='
LEFT_SHIFT ::= '<<'
RIGHT_SHIFT ::= '>>'
UNSIGNED_RIGHT_SHIFT ::= '>>>'
PLUS_EQUAL ::= '+='
MINUS_EQUAL ::= '-='
MULTIPLY_EQUAL ::= '*='
DIVIDE_EQUAL ::= '/='
AND_EQUAL ::= '&='
OR_EQUAL ::= '|='
XOR_EQUAL ::= '^='
REMAINDER_EQUAL ::= '%='
LEFT_SHIFT_EQUAL ::= '<<='
RIGHT_SHIFT_EQUAL ::= '>>='
UNSIGNED_RIGHT_SHIFT_EQUAL ::= '>>>='
OR_OR ::= '||'
AND_AND ::= '&&'
PLUS ::= '+'
MINUS ::= '-'
NOT ::= '!'
REMAINDER ::= '%'
XOR ::= '^'
AND ::= '&'
MULTIPLY ::= '*'
OR ::= '|'
TWIDDLE ::= '~'
DIVIDE ::= '/'
GREATER ::= '>'
LESS ::= '<'
LPAREN ::= '('
RPAREN ::= ')'
LBRACE ::= '{'
RBRACE ::= '}'
LBRACKET ::= '['
RBRACKET ::= ']'
SEMICOLON ::= ';'
QUESTION ::= '?'
COLON ::= ':'
COMMA ::= ','
DOT ::= '.'
EQUAL ::= '='
AT ::= '@'
ELLIPSIS ::= '...'

StatementNoShortIf ::= 'Statement'
StatementWithoutTrailingSubstatement ::= 'Statement'
LabeledStatementNoShortIf ::= 'LabeledStatement'
IfThenElseStatementNoShortIf ::= 'IfThenElseStatement'
WhileStatementNoShortIf ::= 'WhileStatement'
ForStatementNoShortIf ::= 'ForStatement'
ForeachStatementNoShortIf ::= 'ForeachStatement'
UnaryExpressionNotPlusMinus ::= 'UnaryExpression'

PostfixExpressionNotName ::= 'PostfixExpression'
UnaryExpressionNotName ::= 'UnaryExpression'
UnaryExpressionNotPlusMinusNotName ::= 'UnaryExpression'
MultiplicativeExpressionNotName ::= 'MultiplicativeExpression'
AdditiveExpressionNotName ::= 'AdditiveExpression'
ShiftExpressionNotName ::= 'ShiftExpression'
RelationalExpressionNotName ::= 'RelationalExpression'
EqualityExpressionNotName ::= 'EqualityExpression'
AndExpressionNotName ::= 'AndExpression'
ExclusiveOrExpressionNotName ::= 'ExclusiveOrExpression'
InclusiveOrExpressionNotName ::= 'InclusiveOrExpression'
ConditionalAndExpressionNotName ::= 'ConditionalAndExpression'
ConditionalOrExpressionNotName ::= 'ConditionalOrExpression'
ConditionalExpressionNotName ::= 'ConditionalExpression'
AssignmentExpressionNotName ::= 'AssignmentExpression'
ExpressionNotName ::= 'Expression'

Wildcard1 ::= 'Wildcard'
Wildcard2 ::= 'Wildcard'
Wildcard3 ::= 'Wildcard'
TypeArgumentList1 ::= 'TypeArgumentList'
TypeArgumentList2 ::= 'TypeArgumentList'
TypeArgumentList3 ::= 'TypeArgumentList'
TypeArgument1 ::= 'TypeArgument'
TypeArgument2 ::= 'TypeArgument'
TypeArgument3 ::= 'TypeArgument'
ReferenceType1 ::= 'ReferenceType'
ReferenceType2 ::= 'ReferenceType'
ReferenceType3 ::= 'ReferenceType'
TypeParameterList1 ::= 'TypeParameterList'
TypeParameter1 ::= 'TypeParameter'
TypeBound1 ::= 'TypeBound'
AdditionalBoundList1 ::= 'AdditionalBoundList'
AdditionalBound1 ::= 'AdditionalBound'
ClassOrInterfaceType1 ::= 'ClassOrInterfaceType'

$end
