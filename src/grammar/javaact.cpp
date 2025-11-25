#include "platform.h"
#include "parser.h"
#include "ast.h"
#include "stream.h"

#undef HEADERS
#include "javaact.h"


namespace Jopa { // Open namespace Jopa block
//**************************************************************************//
//**************************************************************************//
//*                                                                        *//
//* Below, we show each rule of the Java grammar together with the         *//
//* semantic action that is invoked when the parser performs a reduction   *//
//* by that rule.                                                          *//
//**************************************************************************//
//**************************************************************************//

//
// The parse was bad. Give up now.
//
void Parser::BadAction() { assert(false); }


//
// Rule 1:  Goal ::= CompilationUnit
//
#line 836 "java.g"
//
// Given a rule of the form A ::= x1 x2 ... xn        n >= 1
// Do nothing - Whatever Ast was produced for x1 is inherited by A.
//
void Parser::NoAction() {}
//
// This next rule was added to allow the parser to recognize the body of a
// funtion (constructor, method, or initializer) out of context. Note that
// the artificial terminal BodyMarker is added here to prevent an ordinary
// parse from accepting a body as a valid input - i.e., to recognize a body
// out-of-context, the BodyMarker terminal must be inserted in front of the
// input stream containing the body in question.

//
// Rule 2:  Goal ::= BodyMarker MethodBody
//
#line 854 "java.g"
//
// Given a rule of the form A ::= x1 x2, inherit the result from x2.
//
void Parser::SetSym1ToSym2() { Sym(1) = Sym(2); }

//
// Rule 3:  Literal ::= IntegerLiteral
//
#line 866 "java.g"
void Parser::Act3()
{
    Sym(1) = ast_pool -> NewIntegerLiteral(Token(1));
}

//
// Rule 4:  Literal ::= LongLiteral
//
#line 875 "java.g"
void Parser::Act4()
{
    Sym(1) = ast_pool -> NewLongLiteral(Token(1));
}

//
// Rule 5:  Literal ::= FloatLiteral
//
#line 884 "java.g"
void Parser::Act5()
{
    Sym(1) = ast_pool -> NewFloatLiteral(Token(1));
}

//
// Rule 6:  Literal ::= DoubleLiteral
//
#line 893 "java.g"
void Parser::Act6()
{
    Sym(1) = ast_pool -> NewDoubleLiteral(Token(1));
}

//
// Rule 7:  Literal ::= BooleanLiteral
//
// void NoAction();
//

//
// Rule 8:  Literal ::= CharacterLiteral
//
#line 906 "java.g"
void Parser::Act8()
{
    Sym(1) = ast_pool -> NewCharacterLiteral(Token(1));
}

//
// Rule 9:  Literal ::= StringLiteral
//
#line 915 "java.g"
void Parser::Act9()
{
    Sym(1) = ast_pool -> NewStringLiteral(Token(1));
}

//
// Rule 10:  Literal ::= null
//
#line 924 "java.g"
void Parser::Act10()
{
    Sym(1) = ast_pool -> NewNullLiteral(Token(1));
}

//
// Rule 11:  BooleanLiteral ::= true
//
#line 933 "java.g"
void Parser::Act11()
{
    Sym(1) = ast_pool -> NewTrueLiteral(Token(1));
}

//
// Rule 12:  BooleanLiteral ::= false
//
#line 942 "java.g"
void Parser::Act12()
{
    Sym(1) = ast_pool -> NewFalseLiteral(Token(1));
}

//
// Rule 13:  Type ::= PrimitiveType
//
// void NoAction();
//

//
// Rule 14:  Type ::= ReferenceType
//
// void NoAction();
//

//
// Rule 15:  PrimitiveType ::= NumericType
//
// void NoAction();
//

//
// Rule 16:  PrimitiveType ::= boolean
//
#line 965 "java.g"
void Parser::Act16()
{
    Sym(1) = ast_pool -> NewPrimitiveType(Ast::BOOLEAN, Token(1));
}

//
// Rule 17:  NumericType ::= IntegralType
//
// void NoAction();
//

//
// Rule 18:  NumericType ::= FloatingPointType
//
// void NoAction();
//

//
// Rule 19:  IntegralType ::= byte
//
#line 982 "java.g"
void Parser::Act19()
{
    Sym(1) = ast_pool -> NewPrimitiveType(Ast::BYTE, Token(1));
}

//
// Rule 20:  IntegralType ::= short
//
#line 991 "java.g"
void Parser::Act20()
{
    Sym(1) = ast_pool -> NewPrimitiveType(Ast::SHORT, Token(1));
}

//
// Rule 21:  IntegralType ::= int
//
#line 1000 "java.g"
void Parser::Act21()
{
    Sym(1) = ast_pool -> NewPrimitiveType(Ast::INT, Token(1));
}

//
// Rule 22:  IntegralType ::= long
//
#line 1009 "java.g"
void Parser::Act22()
{
    Sym(1) = ast_pool -> NewPrimitiveType(Ast::LONG, Token(1));
}

//
// Rule 23:  IntegralType ::= char
//
#line 1018 "java.g"
void Parser::Act23()
{
    Sym(1) = ast_pool -> NewPrimitiveType(Ast::CHAR, Token(1));
}

//
// Rule 24:  FloatingPointType ::= float
//
#line 1027 "java.g"
void Parser::Act24()
{
    Sym(1) = ast_pool -> NewPrimitiveType(Ast::FLOAT, Token(1));
}

//
// Rule 25:  FloatingPointType ::= double
//
#line 1036 "java.g"
void Parser::Act25()
{
    Sym(1) = ast_pool -> NewPrimitiveType(Ast::DOUBLE, Token(1));
}

//
// Rule 26:  VoidType ::= void
//
#line 1048 "java.g"
void Parser::Act26()
{
    Sym(1) = ast_pool -> NewPrimitiveType(Ast::VOID_TYPE, Token(1));
}

//
// Rule 27:  ReferenceType ::= ClassOrInterfaceType
//
// void NoAction();
//

//
// Rule 28:  ReferenceType ::= ArrayType
//
// void NoAction();
//

//
// Rule 29:  ClassOrInterfaceType ::= ClassOrInterface
//
// void NoAction();
//

//
// Rule 30:  ClassOrInterfaceType ::= ClassOrInterface LESS TypeArgumentList1 Marker
//
#line 1080 "java.g"
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

//
// Rule 31:  ClassOrInterface ::= Name
//
#line 1112 "java.g"
void Parser::Act31()
{
    AstTypeName* p = ast_pool -> NewTypeName(DYNAMIC_CAST<AstName*> (Sym(1)));
    Sym(1) = p;
}

//
// Rule 32:  ClassOrInterface ::= ClassOrInterface LESS TypeArgumentList1 DOT Name
//
#line 1125 "java.g"
void Parser::Act32()
{
    AstTypeName* p = ast_pool -> NewTypeName(DYNAMIC_CAST<AstName*> (Sym(5)));
    p -> base_opt = MakeTypeArguments(1);
    Sym(1) = p;
}

//
// Rule 33:  ArrayType ::= PrimitiveType Dims
//
#line 1147 "java.g"
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

//
// Rule 34:  ArrayType ::= Name Dims
//
// void MakeArrayType();
//

//
// Rule 35:  ArrayType ::= ClassOrInterface LESS TypeArgumentList1 DOT Name Dims
//
#line 1178 "java.g"
void Parser::Act35()
{
    AstTypeName* p = ast_pool -> NewTypeName(DYNAMIC_CAST<AstName*> (Sym(5)));
    p -> base_opt = MakeTypeArguments(1);
    Sym(5) = p;
    Sym(1) = MakeArrayType(5);
}

//
// Rule 36:  ArrayType ::= ClassOrInterface LESS TypeArgumentList1 Dims
//
#line 1193 "java.g"
void Parser::Act36()
{
    Sym(3) = MakeTypeArguments(1);
    Sym(1) = MakeArrayType(3);
}

//
// Rule 37:  Name ::= Identifier
//
#line 1211 "java.g"
void Parser::Act37() { MakeSimpleName(1); }

//
// Used on "Identifier", and sets the corresponding symbol to a simple name.
//
AstName* Parser::MakeSimpleName(int tokennum)
{
    AstName* name = ast_pool -> NewName(Token(tokennum));
    Sym(tokennum) = name;
    return name;
}

//
// Rule 38:  Name ::= Name DOT Marker Identifier
//
#line 1231 "java.g"
void Parser::Act38()
{
    AstName* p = ast_pool -> NewName(Token(4));
    p -> base_opt = DYNAMIC_CAST<AstName*> (Sym(1));
    Sym(1) = p;
}

//
// Rule 39:  CompilationUnit ::= PackageDeclaration ImportDeclarationsopt TypeDeclarationsopt
//
#line 1252 "java.g"
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

//
// Rule 40:  CompilationUnit ::= Marker ImportDeclarations TypeDeclarationsopt
//
// void MakeCompilationUnit();
//

//
// Rule 41:  CompilationUnit ::= TypeDeclarationsopt
//
#line 1303 "java.g"
void Parser::Act41()
{
    Sym(3) = Sym(1);
    Sym(1) = NULL;
    Sym(2) = NULL;
    MakeCompilationUnit();
}

//
// Rule 42:  ImportDeclarations ::= ImportDeclaration
//
#line 1315 "java.g"
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

//
// Rule 43:  ImportDeclarations ::= ImportDeclarations ImportDeclaration
//
#line 1332 "java.g"
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

//
// Rule 44:  ImportDeclarationsopt ::=
//
#line 1366 "java.g"
//
// Given a rule of the form A ::= x1 x2 ... xn
//
// Construct a NULL Ast for A.
//
void Parser::NullAction() { Sym(1) = NULL; }

//
// Rule 45:  ImportDeclarationsopt ::= ImportDeclarations
//
// void NoAction();
//

//
// Rule 46:  TypeDeclarations ::= TypeDeclaration
//
// void StartList();
//

//
// Rule 47:  TypeDeclarations ::= TypeDeclarations TypeDeclaration
//
// void AddList2();
//

//
// Rule 48:  TypeDeclarationsopt ::=
//
// void NullAction();
//

//
// Rule 49:  TypeDeclarationsopt ::= TypeDeclarations
//
// void NoAction();
//

//
// Rule 50:  PackageDeclaration ::= Marker package Name PackageHeaderMarker SEMICOLON
//
#line 1401 "java.g"
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

//
// Rule 51:  PackageDeclaration ::= Modifiers package Name PackageHeaderMarker SEMICOLON
//
// void MakePackageDeclaration();
//

//
// Rule 52:  ImportDeclaration ::= SingleTypeImportDeclaration
//
// void NoAction();
//

//
// Rule 53:  ImportDeclaration ::= TypeImportOnDemandDeclaration
//
// void NoAction();
//

//
// Rule 54:  ImportDeclaration ::= SingleStaticImportDeclaration
//
// void NoAction();
//

//
// Rule 55:  ImportDeclaration ::= StaticImportOnDemandDeclaration
//
// void NoAction();
//

//
// Rule 56:  SingleTypeImportDeclaration ::= import Marker Name Marker Marker SEMICOLON
//
#line 1482 "java.g"
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

//
// Rule 57:  TypeImportOnDemandDeclaration ::= import Marker Name DOT MULTIPLY SEMICOLON
//
// void MakeImportDeclaration();
//

//
// Rule 58:  SingleStaticImportDeclaration ::= import static Name Marker Marker SEMICOLON
//
// void MakeImportDeclaration();
//

//
// Rule 59:  StaticImportOnDemandDeclaration ::= import static Name DOT MULTIPLY SEMICOLON
//
// void MakeImportDeclaration();
//

//
// Rule 60:  TypeDeclaration ::= ClassDeclaration
//
// void NoAction();
//

//
// Rule 61:  TypeDeclaration ::= EnumDeclaration
//
// void NoAction();
//

//
// Rule 62:  TypeDeclaration ::= InterfaceDeclaration
//
// void NoAction();
//

//
// Rule 63:  TypeDeclaration ::= AnnotationTypeDeclaration
//
// void NoAction();
//

//
// Rule 64:  TypeDeclaration ::= SEMICOLON
//
#line 1556 "java.g"
void Parser::Act64()
{
    Sym(1) = ast_pool -> NewEmptyDeclaration(Token(1));
}

//
// Rule 65:  Modifiers ::= Modifier
//
// void StartList();
//

//
// Rule 66:  Modifiers ::= Modifiers Modifier
//
// void AddList2();
//

//
// Rule 67:  Modifiersopt ::=
//
// void NullAction();
//

//
// Rule 68:  Modifiersopt ::= Modifiers
//
// void NoAction();
//

//
// Rule 69:  Modifier ::= public
//
#line 1593 "java.g"
void Parser::MakeModifier()
{
    Sym(1) = ast_pool -> NewModifierKeyword(Token(1));
}

//
// Rule 70:  Modifier ::= protected
//
// void MakeModifier();
//

//
// Rule 71:  Modifier ::= private
//
// void MakeModifier();
//

//
// Rule 72:  Modifier ::= static
//
// void MakeModifier();
//

//
// Rule 73:  Modifier ::= abstract
//
// void MakeModifier();
//

//
// Rule 74:  Modifier ::= final
//
// void MakeModifier();
//

//
// Rule 75:  Modifier ::= native
//
// void MakeModifier();
//

//
// Rule 76:  Modifier ::= strictfp
//
// void MakeModifier();
//

//
// Rule 77:  Modifier ::= synchronized
//
// void MakeModifier();
//

//
// Rule 78:  Modifier ::= transient
//
// void MakeModifier();
//

//
// Rule 79:  Modifier ::= volatile
//
// void MakeModifier();
//

//
// Rule 80:  Modifier ::= Annotation
//
// void NoAction();
//

//
// Rule 81:  Annotation ::= NormalAnnotation
//
// void NoAction();
//

//
// Rule 82:  Annotation ::= MarkerAnnotation
//
// void NoAction();
//

//
// Rule 83:  Annotation ::= SingleMemberAnnotation
//
// void NoAction();
//

//
// Rule 84:  NormalAnnotation ::= AT Name LPAREN MemberValuePairsopt RPAREN
//
#line 1705 "java.g"
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

//
// Rule 85:  MemberValuePairs ::= MemberValuePair
//
// void StartList();
//

//
// Rule 86:  MemberValuePairs ::= MemberValuePairs COMMA MemberValuePair
//
// void AddList3();
//

//
// Rule 87:  MemberValuePairsopt ::=
//
// void NullAction();
//

//
// Rule 88:  MemberValuePairsopt ::= MemberValuePairs
//
// void NoAction();
//

//
// Rule 89:  MemberValuePair ::= Identifier EQUAL MemberValue
//
#line 1764 "java.g"
void Parser::Act89()
{
    AstMemberValuePair* p = ast_pool -> NewMemberValuePair();
    p -> identifier_token_opt = Token(1);
    p -> member_value = DYNAMIC_CAST<AstMemberValue*> (Sym(3));
    Sym(1) = p;
}

//
// Rule 90:  MemberValue ::= ConditionalExpression
//
// void NoAction();
//

//
// Rule 91:  MemberValue ::= Annotation
//
// void NoAction();
//

//
// Rule 92:  MemberValue ::= MemberValueArrayInitializer
//
// void NoAction();
//

//
// Rule 93:  MemberValueArrayInitializer ::= LBRACE Marker ,opt RBRACE
//
#line 1802 "java.g"
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

//
// Rule 94:  MemberValueArrayInitializer ::= LBRACE MemberValues ,opt RBRACE
//
// void MakeArrayInitializer();
//

//
// Rule 95:  MemberValues ::= MemberValue
//
// void StartList();
//

//
// Rule 96:  MemberValues ::= MemberValues COMMA MemberValue
//
// void AddList3();
//

//
// Rule 97:  MarkerAnnotation ::= AT Name Marker Marker Marker
//
// void MakeAnnotation();
//

//
// Rule 98:  SingleMemberAnnotation ::= AT Name LPAREN MemberValue RPAREN
//
#line 1867 "java.g"
void Parser::Act98()
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

//
// Rule 99:  ClassDeclaration ::= Marker class Identifier TypeParametersopt Superopt Interfacesopt ClassBody
//
#line 1893 "java.g"
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

//
// Rule 100:  ClassDeclaration ::= Modifiers class Identifier TypeParametersopt Superopt Interfacesopt ClassBody
//
// void MakeClassDeclaration();
//

//
// Rule 101:  Super ::= extends ClassOrInterfaceType
//
// void SetSym1ToSym2();
//

//
// Rule 102:  Superopt ::=
//
// void NullAction();
//

//
// Rule 103:  Superopt ::= Super
//
// void NoAction();
//

//
// Rule 104:  Interfaces ::= implements TypeList
//
// void SetSym1ToSym2();
//

//
// Rule 105:  Interfacesopt ::=
//
// void NullAction();
//

//
// Rule 106:  Interfacesopt ::= Interfaces
//
// void NoAction();
//

//
// Rule 107:  TypeList ::= ClassOrInterfaceType
//
// void StartList();
//

//
// Rule 108:  TypeList ::= TypeList COMMA ClassOrInterfaceType
//
// void AddList3();
//

//
// Rule 109:  ClassBody ::= LBRACE ClassBodyDeclarationsopt RBRACE
//
#line 2006 "java.g"
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

//
// Rule 110:  ClassBodyopt ::=
//
// void NullAction();
//

//
// Rule 111:  ClassBodyopt ::= ClassBody
//
// void NoAction();
//

//
// Rule 112:  ClassBodyDeclarations ::= ClassBodyDeclaration
//
// void StartList();
//

//
// Rule 113:  ClassBodyDeclarations ::= ClassBodyDeclarations ClassBodyDeclaration
//
// void AddList2();
//

//
// Rule 114:  ClassBodyDeclarationsopt ::=
//
// void NullAction();
//

//
// Rule 115:  ClassBodyDeclarationsopt ::= ClassBodyDeclarations
//
// void NoAction();
//

//
// Rule 116:  ClassBodyDeclaration ::= ConstructorDeclaration
//
// void NoAction();
//

//
// Rule 117:  ClassBodyDeclaration ::= InitializerDeclaration
//
// void NoAction();
//

//
// Rule 118:  ClassBodyDeclaration ::= FieldDeclaration
//
// void NoAction();
//

//
// Rule 119:  ClassBodyDeclaration ::= MethodDeclaration
//
// void NoAction();
//

//
// Rule 120:  ClassBodyDeclaration ::= TypeDeclaration
//
// void NoAction();
//

//
// Rule 121:  FieldDeclaration ::= Marker Marker Type VariableDeclarators SEMICOLON
//
#line 2187 "java.g"
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

//
// Rule 122:  FieldDeclaration ::= Modifiers Marker Type VariableDeclarators SEMICOLON
//
// void MakeFieldDeclaration();
//

//
// Rule 123:  VariableDeclarators ::= VariableDeclarator
//
// void StartList();
//

//
// Rule 124:  VariableDeclarators ::= VariableDeclarators COMMA VariableDeclarator
//
// void AddList3();
//

//
// Rule 125:  VariableDeclarator ::= VariableDeclaratorId
//
#line 2225 "java.g"
void Parser::Act125()
{
    AstVariableDeclarator* p = ast_pool -> NewVariableDeclarator();
    p -> variable_declarator_name =
        DYNAMIC_CAST<AstVariableDeclaratorId*> (Sym(1));
    Sym(1) = p;
}

//
// Rule 126:  VariableDeclarator ::= VariableDeclaratorId EQUAL VariableInitializer
//
#line 2237 "java.g"
void Parser::Act126()
{
    AstVariableDeclarator* p = ast_pool -> NewVariableDeclarator();
    p -> variable_declarator_name =
        DYNAMIC_CAST<AstVariableDeclaratorId*> (Sym(1));
    p -> variable_initializer_opt = Sym(3);
    Sym(1) = p;
}

//
// Rule 127:  VariableDeclaratorId ::= Identifier Dimsopt
//
#line 2250 "java.g"
void Parser::Act127()
{
    AstVariableDeclaratorId* p = ast_pool -> NewVariableDeclaratorId();
    p -> identifier_token = Token(1);
    p -> brackets_opt = DYNAMIC_CAST<AstBrackets*> (Sym(2));
    Sym(1) = p;
}

//
// Rule 128:  VariableInitializer ::= Expression
//
// void NoAction();
//

//
// Rule 129:  VariableInitializer ::= ArrayInitializer
//
// void NoAction();
//

//
// Rule 130:  MethodDeclaration ::= MethodHeader MethodHeaderMarker MethodBody
//
#line 2282 "java.g"
void Parser::MakeMethodDeclaration()
{
    AstMethodDeclaration* p = DYNAMIC_CAST<AstMethodDeclaration*> (Sym(1));
    if (Sym(3))
        p -> method_body_opt = DYNAMIC_CAST<AstMethodBody*> (Sym(3));
    else p -> semicolon_token_opt = Token(4);
}

//
// Rule 131:  MethodDeclaration ::= MethodHeader MethodHeaderMarker Marker SEMICOLON
//
// void MakeMethodDeclaration();
//

//
// Rule 132:  MethodHeader ::= Marker Marker Type MethodDeclarator Throwsopt
//
#line 2309 "java.g"
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

//
// Rule 133:  MethodHeader ::= Modifiers Marker Type MethodDeclarator Throwsopt
//
// void MakeMethodHeader();
//

//
// Rule 134:  MethodHeader ::= Marker TypeParameters Type MethodDeclarator Throwsopt
//
// void MakeMethodHeader();
//

//
// Rule 135:  MethodHeader ::= Modifiers TypeParameters Type MethodDeclarator Throwsopt
//
// void MakeMethodHeader();
//

//
// Rule 136:  MethodHeader ::= Marker Marker VoidType MethodDeclarator Throwsopt
//
// void MakeMethodHeader();
//

//
// Rule 137:  MethodHeader ::= Modifiers Marker VoidType MethodDeclarator Throwsopt
//
// void MakeMethodHeader();
//

//
// Rule 138:  MethodHeader ::= Marker TypeParameters VoidType MethodDeclarator Throwsopt
//
// void MakeMethodHeader();
//

//
// Rule 139:  MethodHeader ::= Modifiers TypeParameters VoidType MethodDeclarator Throwsopt
//
// void MakeMethodHeader();
//

//
// Rule 140:  MethodDeclarator ::= Identifier LPAREN FormalParameterListopt RPAREN Dimsopt
//
#line 2415 "java.g"
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

//
// Rule 141:  FormalParameterList ::= LastFormalParameter
//
// void StartList();
//

//
// Rule 142:  FormalParameterList ::= FormalParameters COMMA LastFormalParameter
//
// void AddList3();
//

//
// Rule 143:  FormalParameterListopt ::=
//
// void NullAction();
//

//
// Rule 144:  FormalParameterListopt ::= FormalParameterList
//
// void NoAction();
//

//
// Rule 145:  FormalParameters ::= FormalParameter
//
// void StartList();
//

//
// Rule 146:  FormalParameters ::= FormalParameters COMMA FormalParameter
//
// void AddList3();
//

//
// Rule 147:  FormalParameter ::= Type Marker Marker VariableDeclaratorId
//
#line 2486 "java.g"
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

//
// Rule 148:  FormalParameter ::= Modifiers Type Marker VariableDeclaratorId
//
// void MakeFormalParameter();
//

//
// Rule 149:  LastFormalParameter ::= FormalParameter
//
// void NoAction();
//

//
// Rule 150:  LastFormalParameter ::= Type Marker ELLIPSIS VariableDeclaratorId
//
// void MakeFormalParameter();
//

//
// Rule 151:  LastFormalParameter ::= Modifiers Type ELLIPSIS VariableDeclaratorId
//
// void MakeFormalParameter();
//

//
// Rule 152:  Throws ::= throws TypeList
//
// void SetSym1ToSym2();
//

//
// Rule 153:  Throwsopt ::=
//
// void NullAction();
//

//
// Rule 154:  Throwsopt ::= Throws
//
// void NoAction();
//

//
// Rule 155:  MethodBody ::= LBRACE BlockStatementsopt RBRACE
//
#line 2579 "java.g"
void Parser::Act155()
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

//
// Rule 156:  InitializerDeclaration ::= Marker MethodHeaderMarker MethodBody
//
#line 2625 "java.g"
void Parser::MakeInitializerDeclaration()
{
    AstInitializerDeclaration* p = ast_pool -> NewInitializerDeclaration();
    p -> modifiers_opt = MakeModifiers();
    p -> block = DYNAMIC_CAST<AstMethodBody*> (Sym(3));
    Sym(1) = p;
}

//
// Rule 157:  InitializerDeclaration ::= Modifiers MethodHeaderMarker MethodBody
//
// void MakeInitializerDeclaration();
//

//
// Rule 158:  ConstructorDeclaration ::= Marker Marker ConstructorDeclarator Throwsopt MethodHeaderMarker MethodBody
//
#line 2657 "java.g"
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

//
// Rule 159:  ConstructorDeclaration ::= Modifiers Marker ConstructorDeclarator Throwsopt MethodHeaderMarker MethodBody
//
// void MakeConstructorDeclaration();
//

//
// Rule 160:  ConstructorDeclaration ::= Marker TypeParameters ConstructorDeclarator Throwsopt MethodHeaderMarker MethodBody
//
// void MakeConstructorDeclaration();
//

//
// Rule 161:  ConstructorDeclaration ::= Modifiers TypeParameters ConstructorDeclarator Throwsopt MethodHeaderMarker...
//
// void MakeConstructorDeclaration();
//

//
// Rule 162:  ConstructorDeclarator ::= Identifier LPAREN FormalParameterListopt RPAREN Marker
//
// void MakeMethodDeclarator();
//

//
// Rule 163:  ExplicitConstructorInvocation ::= this Arguments SEMICOLON
//
#line 2747 "java.g"
void Parser::Act163()
{
    AstThisCall* p = ast_pool -> NewThisCall();
    p -> this_token = Token(1);
    p -> arguments = DYNAMIC_CAST<AstArguments*> (Sym(2));
    p -> semicolon_token = Token(3);
    Sym(1) = p;
}

//
// Rule 164:  ExplicitConstructorInvocation ::= TypeArguments this Arguments SEMICOLON
//
#line 2763 "java.g"
void Parser::Act164()
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

//
// Rule 165:  ExplicitConstructorInvocation ::= super Arguments SEMICOLON
//
#line 2805 "java.g"
void Parser::Act165()
{
    AstSuperCall* p = ast_pool -> NewSuperCall();
    p -> super_token = Token(1);
    p -> arguments = DYNAMIC_CAST<AstArguments*> (Sym(2));
    p -> semicolon_token = Token(3);
    Sym(1) = p;
}

//
// Rule 166:  ExplicitConstructorInvocation ::= TypeArguments super Arguments SEMICOLON
//
#line 2821 "java.g"
void Parser::Act166()
{
    AstSuperCall* p = ast_pool -> NewSuperCall();
    p -> type_arguments_opt = MakeExplicitTypeArguments(1);
    p -> super_token = Token(2);
    p -> arguments = DYNAMIC_CAST<AstArguments*> (Sym(3));
    p -> semicolon_token = Token(4);
    Sym(1) = p;
}

//
// Rule 167:  ExplicitConstructorInvocation ::= Primary DOT TypeArgumentsopt super Arguments SEMICOLON
//
#line 2842 "java.g"
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

//
// Rule 168:  ExplicitConstructorInvocation ::= Name DOT Marker super Arguments SEMICOLON
//
// void MakeQualifiedSuper();
//

//
// Rule 169:  ExplicitConstructorInvocation ::= Name DOT TypeArguments super Arguments SEMICOLON
//
// void MakeQualifiedSuper();
//

//
// Rule 170:  EnumDeclaration ::= Marker enum Identifier Interfacesopt EnumBody
//
#line 2889 "java.g"
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

//
// Rule 171:  EnumDeclaration ::= Modifiers enum Identifier Interfacesopt EnumBody
//
// void MakeEnumDeclaration();
//

//
// Rule 172:  EnumBody ::= LBRACE Marker ,opt EnumBodyDeclarationsopt RBRACE
//
#line 2935 "java.g"
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

//
// Rule 173:  EnumBody ::= LBRACE EnumConstants ,opt EnumBodyDeclarationsopt RBRACE
//
// void MakeEnumBody();
//

//
// Rule 174:  EnumConstants ::= EnumConstant
//
// void StartList();
//

//
// Rule 175:  EnumConstants ::= EnumConstants COMMA EnumConstant
//
// void AddList3();
//

//
// Rule 176:  EnumConstant ::= Modifiersopt Identifier Argumentsopt ClassBodyopt
//
#line 3001 "java.g"
void Parser::Act176()
{
    AstEnumConstant* p = ast_pool -> NewEnumConstant(Token(2));
    p -> modifiers_opt = MakeModifiers();
    p -> arguments_opt = DYNAMIC_CAST<AstArguments*> (Sym(3));
    p -> class_body_opt = DYNAMIC_CAST<AstClassBody*> (Sym(4));
    Sym(1) = p;
}

//
// Rule 177:  Arguments ::= LPAREN ArgumentListopt RPAREN
//
#line 3018 "java.g"
void Parser::Act177()
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

//
// Rule 178:  Argumentsopt ::=
//
// void NullAction();
//

//
// Rule 179:  Argumentsopt ::= Arguments
//
// void NoAction();
//

//
// Rule 180:  EnumBodyDeclarations ::= SEMICOLON ClassBodyDeclarationsopt Marker
//
// void MakeClassBody();
//

//
// Rule 181:  EnumBodyDeclarationsopt ::=
//
// void NullAction();
//

//
// Rule 182:  EnumBodyDeclarationsopt ::= EnumBodyDeclarations
//
// void NoAction();
//

//
// Rule 183:  InterfaceDeclaration ::= Marker interface Identifier TypeParametersopt ExtendsInterfacesopt InterfaceBody
//
#line 3088 "java.g"
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

//
// Rule 184:  InterfaceDeclaration ::= Modifiers interface Identifier TypeParametersopt ExtendsInterfacesopt InterfaceBody
//
// void MakeInterfaceDeclaration();
//

//
// Rule 185:  ExtendsInterfaces ::= extends TypeList
//
// void SetSym1ToSym2();
//

//
// Rule 186:  ExtendsInterfacesopt ::=
//
// void NullAction();
//

//
// Rule 187:  ExtendsInterfacesopt ::= ExtendsInterfaces
//
// void NoAction();
//

//
// Rule 188:  InterfaceBody ::= LBRACE InterfaceMemberDeclarationsopt RBRACE
//
// void MakeClassBody();
//

//
// Rule 189:  InterfaceMemberDeclarations ::= InterfaceMemberDeclaration
//
// void StartList();
//

//
// Rule 190:  InterfaceMemberDeclarations ::= InterfaceMemberDeclarations InterfaceMemberDeclaration
//
// void AddList2();
//

//
// Rule 191:  InterfaceMemberDeclarationsopt ::=
//
// void NullAction();
//

//
// Rule 192:  InterfaceMemberDeclarationsopt ::= InterfaceMemberDeclarations
//
// void NoAction();
//

//
// Rule 193:  InterfaceMemberDeclaration ::= ConstantDeclaration
//
// void NoAction();
//

//
// Rule 194:  InterfaceMemberDeclaration ::= TypeDeclaration
//
// void NoAction();
//

//
// Rule 195:  ConstantDeclaration ::= FieldDeclaration
//
#line 3197 "java.g"
void Parser::Act195()
{
    DYNAMIC_CAST<AstFieldDeclaration*> (Sym(1)) -> MarkStatic();
}

//
// Rule 196:  InterfaceMemberDeclaration ::= MethodDeclaration
//
// void NoAction();
//

//
// Rule 197:  AnnotationTypeDeclaration ::= AT Marker interface Identifier AnnotationTypeBody
//
#line 3222 "java.g"
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

//
// Rule 198:  AnnotationTypeDeclaration ::= Modifiers AT interface Identifier AnnotationTypeBody
//
// void MakeAnnotationTypeDeclaration();
//

//
// Rule 199:  AnnotationTypeBody ::= LBRACE AnnotationTypeMemberDeclarationsopt RBRACE
//
// void MakeClassBody();
//

//
// Rule 200:  AnnotationTypeMemberDeclarations ::= AnnotationTypeMemberDeclaration
//
// void StartList();
//

//
// Rule 201:  AnnotationTypeMemberDeclarations ::= AnnotationTypeMemberDeclarations AnnotationTypeMemberDeclaration
//
// void AddList2();
//

//
// Rule 202:  AnnotationTypeMemberDeclarationsopt ::=
//
// void NullAction();
//

//
// Rule 203:  AnnotationTypeMemberDeclarationsopt ::= AnnotationTypeMemberDeclarations
//
// void NoAction();
//

//
// Rule 204:  AnnotationTypeMemberDeclaration ::= Marker Marker Type Identifier LPAREN RPAREN DefaultValueopt SEMICOLON
//
#line 3296 "java.g"
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

//
// Rule 205:  AnnotationTypeMemberDeclaration ::= Modifiers Marker Type Identifier LPAREN RPAREN DefaultValueopt SEMICOLON
//
// void MakeAnnotationTypeMemberDeclaration();
//

//
// Rule 206:  AnnotationTypeMemberDeclaration ::= ConstantDeclaration
//
// void NoAction();
//

//
// Rule 207:  AnnotationTypeMemberDeclaration ::= TypeDeclaration
//
// void NoAction();
//

//
// Rule 208:  DefaultValue ::= default MemberValue
//
// void SetSym1ToSym2();
//

//
// Rule 209:  DefaultValueopt ::=
//
// void NullAction();
//

//
// Rule 210:  DefaultValueopt ::= DefaultValue
//
// void NoAction();
//

//
// Rule 211:  ArrayInitializer ::= LBRACE Marker ,opt RBRACE
//
// void MakeArrayInitializer();
//

//
// Rule 212:  ArrayInitializer ::= LBRACE VariableInitializers ,opt RBRACE
//
// void MakeArrayInitializer();
//

//
// Rule 213:  VariableInitializers ::= VariableInitializer
//
// void StartList();
//

//
// Rule 214:  VariableInitializers ::= VariableInitializers COMMA VariableInitializer
//
// void AddList3();
//

//
// Rule 215:  Block ::= LBRACE BlockStatementsopt RBRACE
//
#line 3400 "java.g"
void Parser::Act215()
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

//
// Rule 216:  BlockStatements ::= BlockStatement
//
// void StartList();
//

//
// Rule 217:  BlockStatements ::= BlockStatements BlockStatement
//
// void AddList2();
//

//
// Rule 218:  BlockStatementsopt ::=
//
// void NullAction();
//

//
// Rule 219:  BlockStatementsopt ::= BlockStatements
//
// void NoAction();
//

//
// Rule 220:  BlockStatement ::= LocalVariableDeclarationStatement
//
// void NoAction();
//

//
// Rule 221:  BlockStatement ::= Statement
//
// void NoAction();
//

//
// Rule 222:  BlockStatement ::= ClassDeclaration
//
#line 3449 "java.g"
void Parser::Act222()
{
    Sym(1) = ast_pool ->
        NewLocalClassStatement(DYNAMIC_CAST<AstClassDeclaration*> (Sym(1)));
}

//
// Rule 223:  BlockStatement ::= EnumDeclaration
//
#line 3462 "java.g"
void Parser::Act223()
{
    Sym(1) = ast_pool ->
        NewLocalClassStatement(DYNAMIC_CAST<AstEnumDeclaration*> (Sym(1)));
}

//
// Rule 224:  BlockStatement ::= ExplicitConstructorInvocation
//
// void NoAction();
//

//
// Rule 225:  LocalVariableDeclarationStatement ::= LocalVariableDeclaration SEMICOLON
//
#line 3481 "java.g"
void Parser::Act225()
{
    DYNAMIC_CAST<AstLocalVariableStatement*> (Sym(1)) -> semicolon_token_opt =
        Token(2);
}

//
// Rule 226:  LocalVariableDeclaration ::= Type Marker Marker VariableDeclarators
//
#line 3497 "java.g"
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

//
// Rule 227:  LocalVariableDeclaration ::= Modifiers Type Marker VariableDeclarators
//
// void MakeLocalVariable();
//

//
// Rule 228:  Statement ::= StatementWithoutTrailingSubstatement
//
// void NoAction();
//

//
// Rule 229:  Statement ::= LabeledStatement
//
// void NoAction();
//

//
// Rule 230:  Statement ::= IfThenStatement
//
// void NoAction();
//

//
// Rule 231:  Statement ::= IfThenElseStatement
//
// void NoAction();
//

//
// Rule 232:  Statement ::= WhileStatement
//
// void NoAction();
//

//
// Rule 233:  Statement ::= ForStatement
//
// void NoAction();
//

//
// Rule 234:  Statement ::= ForeachStatement
//
// void NoAction();
//

//
// Rule 235:  StatementNoShortIf ::= StatementWithoutTrailingSubstatement
//
// void NoAction();
//

//
// Rule 236:  StatementNoShortIf ::= LabeledStatementNoShortIf
//
// void NoAction();
//

//
// Rule 237:  StatementNoShortIf ::= IfThenElseStatementNoShortIf
//
// void NoAction();
//

//
// Rule 238:  StatementNoShortIf ::= WhileStatementNoShortIf
//
// void NoAction();
//

//
// Rule 239:  StatementNoShortIf ::= ForStatementNoShortIf
//
// void NoAction();
//

//
// Rule 240:  StatementNoShortIf ::= ForeachStatementNoShortIf
//
// void NoAction();
//

//
// Rule 241:  StatementWithoutTrailingSubstatement ::= Block
//
// void NoAction();
//

//
// Rule 242:  StatementWithoutTrailingSubstatement ::= EmptyStatement
//
// void NoAction();
//

//
// Rule 243:  StatementWithoutTrailingSubstatement ::= ExpressionStatement
//
// void NoAction();
//

//
// Rule 244:  StatementWithoutTrailingSubstatement ::= SwitchStatement
//
// void NoAction();
//

//
// Rule 245:  StatementWithoutTrailingSubstatement ::= DoStatement
//
// void NoAction();
//

//
// Rule 246:  StatementWithoutTrailingSubstatement ::= BreakStatement
//
// void NoAction();
//

//
// Rule 247:  StatementWithoutTrailingSubstatement ::= ContinueStatement
//
// void NoAction();
//

//
// Rule 248:  StatementWithoutTrailingSubstatement ::= ReturnStatement
//
// void NoAction();
//

//
// Rule 249:  StatementWithoutTrailingSubstatement ::= SynchronizedStatement
//
// void NoAction();
//

//
// Rule 250:  StatementWithoutTrailingSubstatement ::= ThrowStatement
//
// void NoAction();
//

//
// Rule 251:  StatementWithoutTrailingSubstatement ::= TryStatement
//
// void NoAction();
//

//
// Rule 252:  StatementWithoutTrailingSubstatement ::= AssertStatement
//
// void NoAction();
//

//
// Rule 253:  EmptyStatement ::= SEMICOLON
//
#line 3645 "java.g"
void Parser::Act253()
{
    Sym(1) = ast_pool -> NewEmptyStatement(Token(1));
}

//
// Rule 254:  LabeledStatement ::= Identifier COLON Statement
//
#line 3654 "java.g"
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

//
// Rule 255:  LabeledStatementNoShortIf ::= Identifier COLON StatementNoShortIf
//
// void MakeLabeledStatement();
//

//
// Rule 256:  ExpressionStatement ::= StatementExpression SEMICOLON
//
#line 3688 "java.g"
void Parser::Act256()
{
    DYNAMIC_CAST<AstExpressionStatement*> (Sym(1)) -> semicolon_token_opt =
        Token(2);
}

//
// Rule 257:  StatementExpression ::= Assignment
//
#line 3698 "java.g"
void Parser::MakeExpressionStatement()
{
    AstExpressionStatement* p = ast_pool -> NewExpressionStatement();
    p -> expression = DYNAMIC_CAST<AstExpression*> (Sym(1));
    Sym(1) = p;
}

//
// Rule 258:  StatementExpression ::= PreIncrementExpression
//
// void MakeExpressionStatement();
//

//
// Rule 259:  StatementExpression ::= PreDecrementExpression
//
// void MakeExpressionStatement();
//

//
// Rule 260:  StatementExpression ::= PostIncrementExpression
//
// void MakeExpressionStatement();
//

//
// Rule 261:  StatementExpression ::= PostDecrementExpression
//
// void MakeExpressionStatement();
//

//
// Rule 262:  StatementExpression ::= MethodInvocation
//
// void MakeExpressionStatement();
//

//
// Rule 263:  StatementExpression ::= ClassInstanceCreationExpression
//
// void MakeExpressionStatement();
//

//
// Rule 264:  IfThenStatement ::= if LPAREN Expression RPAREN Statement Marker Marker
//
#line 3755 "java.g"
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

//
// Rule 265:  IfThenElseStatement ::= if LPAREN Expression RPAREN StatementNoShortIf else Statement
//
// void MakeIfThenElseStatement();
//

//
// Rule 266:  IfThenElseStatementNoShortIf ::= if LPAREN Expression RPAREN StatementNoShortIf else StatementNoShortIf
//
// void MakeIfThenElseStatement();
//

//
// Rule 267:  SwitchStatement ::= switch LPAREN Expression RPAREN SwitchBlock
//
#line 3807 "java.g"
void Parser::Act267()
{
    AstSwitchStatement* p = DYNAMIC_CAST<AstSwitchStatement*> (Sym(5));
    p -> switch_token = Token(1);
    p -> expression = DYNAMIC_CAST<AstExpression*> (Sym(3));
    Sym(1) = p;
}

//
// Rule 268:  SwitchBlock ::= LBRACE SwitchBlockStatements SwitchLabelsopt RBRACE
//
#line 3825 "java.g"
void Parser::Act268()
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

//
// Rule 269:  SwitchBlock ::= LBRACE SwitchLabelsopt RBRACE
//
#line 3893 "java.g"
void Parser::Act269()
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

//
// Rule 270:  SwitchBlockStatements ::= SwitchBlockStatement
//
// void StartList();
//

//
// Rule 271:  SwitchBlockStatements ::= SwitchBlockStatements SwitchBlockStatement
//
// void AddList2();
//

//
// Rule 272:  SwitchBlockStatement ::= SwitchLabels BlockStatements
//
#line 3922 "java.g"
void Parser::Act272()
{
    Sym(1) = MakeSwitchBlockStatement(DYNAMIC_CAST<AstListNode*> (Sym(1)),
                                      DYNAMIC_CAST<AstListNode*> (Sym(2)));
}

//
// Rule 273:  SwitchLabels ::= SwitchLabel
//
// void StartList();
//

//
// Rule 274:  SwitchLabels ::= SwitchLabels SwitchLabel
//
// void AddList2();
//

//
// Rule 275:  SwitchLabelsopt ::=
//
// void NullAction();
//

//
// Rule 276:  SwitchLabelsopt ::= SwitchLabels
//
// void NoAction();
//

//
// Rule 277:  SwitchLabel ::= case Expression COLON
//
#line 3952 "java.g"
void Parser::MakeSwitchLabel()
{
    AstSwitchLabel* p = ast_pool -> NewSwitchLabel();
    p -> case_token = Token(1);
    p -> expression_opt = DYNAMIC_CAST<AstExpression*> (Sym(2));
    p -> colon_token = Token(3);
    Sym(1) = p;
}

//
// Rule 278:  SwitchLabel ::= default Marker COLON
//
// void MakeSwitchLabel();
//

//
// Rule 279:  WhileStatement ::= while LPAREN Expression RPAREN Statement
//
#line 3976 "java.g"
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

//
// Rule 280:  WhileStatementNoShortIf ::= while LPAREN Expression RPAREN StatementNoShortIf
//
// void MakeWhileStatement();
//

//
// Rule 281:  DoStatement ::= do Statement while LPAREN Expression RPAREN SEMICOLON
//
#line 4008 "java.g"
void Parser::Act281()
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

//
// Rule 282:  ForStatement ::= for LPAREN ForInitopt SEMICOLON Expressionopt SEMICOLON ForUpdateopt RPAREN Statement
//
#line 4036 "java.g"
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

//
// Rule 283:  ForStatementNoShortIf ::= for LPAREN ForInitopt SEMICOLON Expressionopt SEMICOLON ForUpdateopt RPAREN...
//
// void MakeForStatement();
//

//
// Rule 284:  ForInit ::= StatementExpressionList
//
// void NoAction();
//

//
// Rule 285:  ForInit ::= LocalVariableDeclaration
//
// void StartList();
//

//
// Rule 286:  ForInitopt ::=
//
// void NullAction();
//

//
// Rule 287:  ForInitopt ::= ForInit
//
// void NoAction();
//

//
// Rule 288:  ForUpdate ::= StatementExpressionList
//
// void NoAction();
//

//
// Rule 289:  ForUpdateopt ::=
//
// void NullAction();
//

//
// Rule 290:  ForUpdateopt ::= ForUpdate
//
// void NoAction();
//

//
// Rule 291:  StatementExpressionList ::= StatementExpression
//
// void StartList();
//

//
// Rule 292:  StatementExpressionList ::= StatementExpressionList COMMA StatementExpression
//
// void AddList3();
//

//
// Rule 293:  ForeachStatement ::= for LPAREN FormalParameter COLON Expression RPAREN Statement
//
#line 4133 "java.g"
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

//
// Rule 294:  ForeachStatementNoShortIf ::= for LPAREN FormalParameter COLON Expression RPAREN StatementNoShortIf
//
// void MakeForeachStatement();
//

//
// Rule 295:  AssertStatement ::= assert Expression Marker Marker SEMICOLON
//
#line 4175 "java.g"
void Parser::MakeAssertStatement()
{
    AstAssertStatement* p = ast_pool -> NewAssertStatement();
    p -> assert_token = Token(1);
    p -> condition = DYNAMIC_CAST<AstExpression*> (Sym(2));
    p -> message_opt = DYNAMIC_CAST<AstExpression*> (Sym(4));
    p -> semicolon_token = Token(5);
    Sym(1) = p;
}

//
// Rule 296:  AssertStatement ::= assert Expression COLON Expression SEMICOLON
//
// void MakeAssertStatement();
//

//
// Rule 297:  BreakStatement ::= break Identifieropt SEMICOLON
//
#line 4196 "java.g"
void Parser::Act297()
{
    AstBreakStatement* p = ast_pool -> NewBreakStatement();
    p -> break_token = Token(1);
    if (Token(3) > Token(2))
        p -> identifier_token_opt = Token(2);
    p -> semicolon_token = Token(3);
    Sym(1) = p;
}

//
// Rule 298:  ContinueStatement ::= continue Identifieropt SEMICOLON
//
#line 4210 "java.g"
void Parser::Act298()
{
    AstContinueStatement* p = ast_pool -> NewContinueStatement();
    p -> continue_token = Token(1);
    if (Token(3) > Token(2))
        p -> identifier_token_opt = Token(2);
    p -> semicolon_token = Token(3);
    Sym(1) = p;
}

//
// Rule 299:  ReturnStatement ::= return Expressionopt SEMICOLON
//
#line 4224 "java.g"
void Parser::Act299()
{
    AstReturnStatement* p = ast_pool -> NewReturnStatement();
    p -> return_token = Token(1);
    p -> expression_opt = DYNAMIC_CAST<AstExpression*> (Sym(2));
    p -> semicolon_token = Token(3);
    Sym(1) = p;
}

//
// Rule 300:  ThrowStatement ::= throw Expression SEMICOLON
//
#line 4237 "java.g"
void Parser::Act300()
{
    AstThrowStatement* p = ast_pool -> NewThrowStatement();
    p -> throw_token = Token(1);
    p -> expression = DYNAMIC_CAST<AstExpression*> (Sym(2));
    p -> semicolon_token = Token(3);
    Sym(1) = p;
}

//
// Rule 301:  SynchronizedStatement ::= synchronized LPAREN Expression RPAREN Block
//
#line 4250 "java.g"
void Parser::Act301()
{
    AstSynchronizedStatement* p = ast_pool -> NewSynchronizedStatement();
    p -> synchronized_token = Token(1);
    p -> expression = DYNAMIC_CAST<AstExpression*> (Sym(3));
    p -> block = DYNAMIC_CAST<AstBlock*> (Sym(5));
    p -> block -> SetTag(AstBlock::SYNCHRONIZED);

    Sym(1) = p;
}

//
// Rule 302:  TryStatement ::= try Block Catches Marker
//
#line 4269 "java.g"
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

//
// Rule 303:  TryStatement ::= try Block Catchesopt Finally
//
// void MakeTryStatement();
//

//
// Rule 304:  Catches ::= CatchClause
//
// void StartList();
//

//
// Rule 305:  Catches ::= Catches CatchClause
//
// void AddList2();
//

//
// Rule 306:  Catchesopt ::=
//
// void NullAction();
//

//
// Rule 307:  Catchesopt ::= Catches
//
// void NoAction();
//

//
// Rule 308:  CatchClause ::= catch LPAREN FormalParameter RPAREN Block
//
#line 4325 "java.g"
void Parser::Act308()
{
    AstCatchClause* p = ast_pool -> NewCatchClause();
    p -> catch_token = Token(1);
    p -> formal_parameter = DYNAMIC_CAST<AstFormalParameter*> (Sym(3));
    p -> block = DYNAMIC_CAST<AstBlock*> (Sym(5));

    Sym(1) = p;
}

//
// Rule 309:  Finally ::= finally Block
//
#line 4339 "java.g"
void Parser::Act309()
{
    AstFinallyClause* p = ast_pool -> NewFinallyClause();
    p -> finally_token = Token(1);
    p -> block = DYNAMIC_CAST<AstBlock*> (Sym(2));
    p -> block -> SetTag(AstBlock::FINALLY);

    Sym(1) = p;
}

//
// Rule 310:  Primary ::= PrimaryNoNewArray
//
// void NoAction();
//

//
// Rule 311:  Primary ::= ArrayCreationUninitialized
//
// void NoAction();
//

//
// Rule 312:  Primary ::= ArrayCreationInitialized
//
// void NoAction();
//

//
// Rule 313:  PrimaryNoNewArray ::= Literal
//
// void NoAction();
//

//
// Rule 314:  PrimaryNoNewArray ::= this
//
#line 4378 "java.g"
void Parser::Act314()
{
    Sym(1) = ast_pool -> NewThisExpression(Token(1));
}

//
// Rule 315:  PrimaryNoNewArray ::= LPAREN Name Marker RPAREN
//
#line 4395 "java.g"
void Parser::MakeParenthesizedExpression()
{
    AstParenthesizedExpression* p = ast_pool -> NewParenthesizedExpression();
    p -> left_parenthesis_token = Token(1);
    p -> expression = DYNAMIC_CAST<AstExpression*> (Sym(2));
    p -> right_parenthesis_token = Token(4);
    Sym(1) = p;
}

//
// Rule 316:  PrimaryNoNewArray ::= LPAREN ExpressionNotName Marker RPAREN
//
// void MakeParenthesizedExpression();
//

//
// Rule 317:  PrimaryNoNewArray ::= ClassInstanceCreationExpression
//
// void NoAction();
//

//
// Rule 318:  PrimaryNoNewArray ::= FieldAccess
//
// void NoAction();
//

//
// Rule 319:  PrimaryNoNewArray ::= Name DOT this
//
#line 4431 "java.g"
void Parser::Act319()
{
    AstThisExpression* p = ast_pool -> NewThisExpression(Token(3));
    p -> base_opt = ast_pool -> NewTypeName(DYNAMIC_CAST<AstName*> (Sym(1)));
    Sym(1) = p;
}

//
// Rule 320:  PrimaryNoNewArray ::= PrimitiveType Dimsopt DOT class
//
#line 4447 "java.g"
void Parser::MakeClassLiteral()
{
    AstClassLiteral* p = ast_pool -> NewClassLiteral(Token(4));
    if (Token(3) == Token(4))
        Sym(2) = NULL;
    p -> type = MakeArrayType(1);
    Sym(1) = p;
}

//
// Rule 321:  PrimaryNoNewArray ::= Name Dims DOT class
//
// void MakeClassLiteral();
//

//
// Rule 322:  PrimaryNoNewArray ::= Name DOT Marker class
//
// void MakeClassLiteral();
//

//
// Rule 323:  PrimaryNoNewArray ::= VoidType DOT Marker class
//
// void MakeClassLiteral();
//

//
// Rule 324:  PrimaryNoNewArray ::= MethodInvocation
//
// void NoAction();
//

//
// Rule 325:  PrimaryNoNewArray ::= ArrayAccess
//
// void NoAction();
//

//
// Rule 326:  ClassInstanceCreationExpression ::= new ClassOrInterfaceType Arguments ClassBodyopt
//
#line 4505 "java.g"
void Parser::Act326()
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

//
// Rule 327:  ClassInstanceCreationExpression ::= new TypeArguments ClassOrInterfaceType Arguments ClassBodyopt
//
#line 4528 "java.g"
void Parser::Act327()
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

//
// Rule 328:  ClassInstanceCreationExpression ::= Primary DOT new TypeArgumentsopt Identifier TypeArgumentsopt Arguments...
//
#line 4554 "java.g"
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

//
// Rule 329:  ClassInstanceCreationExpression ::= Name DOT new TypeArgumentsopt Identifier TypeArgumentsopt Arguments...
//
// void MakeQualifiedNew();
//

//
// Rule 330:  ArgumentList ::= Expression
//
// void StartList();
//

//
// Rule 331:  ArgumentList ::= ArgumentList COMMA Expression
//
// void AddList3();
//

//
// Rule 332:  ArgumentListopt ::=
//
// void NullAction();
//

//
// Rule 333:  ArgumentListopt ::= ArgumentList
//
// void NoAction();
//

//
// Rule 334:  ArrayCreationUninitialized ::= new PrimitiveType DimExprs Dimsopt
//
#line 4610 "java.g"
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

//
// Rule 335:  ArrayCreationUninitialized ::= new ClassOrInterfaceType DimExprs Dimsopt
//
// void MakeArrayCreationUninitialized();
//

//
// Rule 336:  ArrayCreationInitialized ::= new PrimitiveType Dims ArrayInitializer
//
#line 4643 "java.g"
void Parser::MakeArrayCreationInitialized()
{
    AstArrayCreationExpression* p = ast_pool -> NewArrayCreationExpression();
    p -> new_token = Token(1);
    p -> array_type = MakeArrayType(2);
    p -> array_initializer_opt = DYNAMIC_CAST<AstArrayInitializer*> (Sym(4));
    Sym(1) = p;
}

//
// Rule 337:  ArrayCreationInitialized ::= new ClassOrInterfaceType Dims ArrayInitializer
//
// void MakeArrayCreationInitialized();
//

//
// Rule 338:  DimExprs ::= DimExpr
//
// void StartList();
//

//
// Rule 339:  DimExprs ::= DimExprs DimExpr
//
// void AddList2();
//

//
// Rule 340:  DimExpr ::= LBRACKET Expression RBRACKET
//
#line 4671 "java.g"
void Parser::Act340()
{
    AstDimExpr* p = ast_pool -> NewDimExpr();
    p -> left_bracket_token = Token(1);
    p -> expression = DYNAMIC_CAST<AstExpression*> (Sym(2));
    p -> right_bracket_token = Token(3);
    Sym(1) = p;
}

//
// Rule 341:  Dims ::= LBRACKET RBRACKET
//
#line 4684 "java.g"
void Parser::Act341()
{
    Sym(1) = ast_pool -> NewBrackets(Token(1), Token(2));
}

//
// Rule 342:  Dims ::= Dims LBRACKET RBRACKET
//
#line 4693 "java.g"
void Parser::Act342()
{
    AstBrackets* p = DYNAMIC_CAST<AstBrackets*> (Sym(1));
    p -> right_bracket_token = Token(2);
    p -> dims++;
}

//
// Rule 343:  Dimsopt ::=
//
// void NullAction();
//

//
// Rule 344:  Dimsopt ::= Dims
//
// void NoAction();
//

//
// Rule 345:  SuperAccess ::= super
//
#line 4715 "java.g"
void Parser::Act345()
{
    Sym(1) = ast_pool -> NewSuperExpression(Token(1));
}

//
// Rule 346:  SuperAccess ::= Name DOT Marker super
//
#line 4730 "java.g"
void Parser::Act346()
{
    AstSuperExpression* p = ast_pool -> NewSuperExpression(Token(4));
    p -> base_opt = ast_pool -> NewTypeName(DYNAMIC_CAST<AstName*> (Sym(1)));
    Sym(1) = p;
}

//
// Rule 347:  FieldAccess ::= Primary DOT Marker Identifier
//
#line 4745 "java.g"
void Parser::MakeFieldAccess()
{
    AstFieldAccess* p = ast_pool -> NewFieldAccess();
    p -> base = DYNAMIC_CAST<AstExpression*> (Sym(1));
    p -> identifier_token = Token(4);
    Sym(1) = p;
}

//
// Rule 348:  FieldAccess ::= SuperAccess DOT Marker Identifier
//
// void MakeFieldAccess();
//

//
// Rule 349:  MethodInvocation ::= Identifier Arguments
//
#line 4775 "java.g"
void Parser::Act349()
{
    AstMethodInvocation* p = ast_pool -> NewMethodInvocation(Token(1));
    p -> arguments = DYNAMIC_CAST<AstArguments*> (Sym(2));
    Sym(1) = p;
}

//
// Rule 350:  MethodInvocation ::= Name DOT Marker Identifier Arguments
//
#line 4791 "java.g"
void Parser::MakeMethodInvocation()
{
    AstMethodInvocation* p = ast_pool -> NewMethodInvocation(Token(4));
    p -> base_opt = DYNAMIC_CAST<AstExpression*> (Sym(1));
    p -> type_arguments_opt = MakeExplicitTypeArguments(3);
    p -> arguments = DYNAMIC_CAST<AstArguments*> (Sym(5));
    Sym(1) = p;
}

//
// Rule 351:  MethodInvocation ::= Name DOT TypeArguments Identifier Arguments
//
// void MakeMethodInvocation();
//

//
// Rule 352:  MethodInvocation ::= Primary DOT Marker Identifier Arguments
//
// void MakeMethodInvocation();
//

//
// Rule 353:  MethodInvocation ::= Primary DOT TypeArguments Identifier Arguments
//
// void MakeMethodInvocation();
//

//
// Rule 354:  MethodInvocation ::= SuperAccess DOT Marker Identifier Arguments
//
// void MakeMethodInvocation();
//

//
// Rule 355:  MethodInvocation ::= SuperAccess DOT TypeArguments Identifier Arguments
//
// void MakeMethodInvocation();
//

//
// Rule 356:  ArrayAccess ::= Name LBRACKET Expression RBRACKET
//
#line 4864 "java.g"
void Parser::MakeArrayAccess()
{
    AstArrayAccess* p = ast_pool -> NewArrayAccess();
    p -> base = DYNAMIC_CAST<AstExpression*> (Sym(1));
    p -> left_bracket_token = Token(2);
    p -> expression = DYNAMIC_CAST<AstExpression*> (Sym(3));
    p -> right_bracket_token = Token(4);
    Sym(1) = p;
}

//
// Rule 357:  ArrayAccess ::= PrimaryNoNewArray LBRACKET Expression RBRACKET
//
// void MakeArrayAccess();
//

//
// Rule 358:  ArrayAccess ::= ArrayCreationInitialized LBRACKET Expression RBRACKET
//
// void MakeArrayAccess();
//

//
// Rule 359:  PostfixExpression ::= Primary
//
// void NoAction();
//

//
// Rule 360:  PostfixExpression ::= Name
//
// void NoAction();
//

//
// Rule 361:  PostfixExpression ::= PostIncrementExpression
//
// void NoAction();
//

//
// Rule 362:  PostfixExpression ::= PostDecrementExpression
//
// void NoAction();
//

//
// Rule 363:  PostfixExpressionNotName ::= Primary
//
// void NoAction();
//

//
// Rule 364:  PostfixExpressionNotName ::= PostIncrementExpression
//
// void NoAction();
//

//
// Rule 365:  PostfixExpressionNotName ::= PostDecrementExpression
//
// void NoAction();
//

//
// Rule 366:  PostIncrementExpression ::= PostfixExpression PLUS_PLUS
//
#line 4924 "java.g"
void Parser::Act366()
{
    AstPostUnaryExpression* p =
        ast_pool -> NewPostUnaryExpression(AstPostUnaryExpression::PLUSPLUS);
    p -> expression = DYNAMIC_CAST<AstExpression*> (Sym(1));
    p -> post_operator_token = Token(2);
    Sym(1) = p;
}

//
// Rule 367:  PostDecrementExpression ::= PostfixExpression MINUS_MINUS
//
#line 4937 "java.g"
void Parser::Act367()
{
    AstPostUnaryExpression* p =
        ast_pool -> NewPostUnaryExpression(AstPostUnaryExpression::MINUSMINUS);
    p -> expression = DYNAMIC_CAST<AstExpression*> (Sym(1));
    p -> post_operator_token = Token(2);
    Sym(1) = p;
}

//
// Rule 368:  UnaryExpression ::= PreIncrementExpression
//
// void NoAction();
//

//
// Rule 369:  UnaryExpression ::= PreDecrementExpression
//
// void NoAction();
//

//
// Rule 370:  UnaryExpression ::= PLUS UnaryExpression
//
#line 4958 "java.g"
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

//
// Rule 371:  UnaryExpression ::= MINUS UnaryExpression
//
// void MakePreUnaryExpression();
//

//
// Rule 372:  UnaryExpression ::= UnaryExpressionNotPlusMinus
//
// void NoAction();
//

//
// Rule 373:  UnaryExpressionNotName ::= PreIncrementExpression
//
// void NoAction();
//

//
// Rule 374:  UnaryExpressionNotName ::= PreDecrementExpression
//
// void NoAction();
//

//
// Rule 375:  UnaryExpressionNotName ::= PLUS UnaryExpression
//
// void MakePreUnaryExpression();
//

//
// Rule 376:  UnaryExpressionNotName ::= MINUS UnaryExpression
//
// void MakePreUnaryExpression();
//

//
// Rule 377:  UnaryExpressionNotName ::= UnaryExpressionNotPlusMinusNotName
//
// void NoAction();
//

//
// Rule 378:  PreIncrementExpression ::= PLUS_PLUS UnaryExpression
//
// void MakePreUnaryExpression();
//

//
// Rule 379:  PreDecrementExpression ::= MINUS_MINUS UnaryExpression
//
// void MakePreUnaryExpression();
//

//
// Rule 380:  UnaryExpressionNotPlusMinus ::= PostfixExpression
//
// void NoAction();
//

//
// Rule 381:  UnaryExpressionNotPlusMinus ::= TWIDDLE UnaryExpression
//
// void MakePreUnaryExpression();
//

//
// Rule 382:  UnaryExpressionNotPlusMinus ::= NOT UnaryExpression
//
// void MakePreUnaryExpression();
//

//
// Rule 383:  UnaryExpressionNotPlusMinus ::= CastExpression
//
// void NoAction();
//

//
// Rule 384:  UnaryExpressionNotPlusMinusNotName ::= PostfixExpressionNotName
//
// void NoAction();
//

//
// Rule 385:  UnaryExpressionNotPlusMinusNotName ::= TWIDDLE UnaryExpression
//
// void MakePreUnaryExpression();
//

//
// Rule 386:  UnaryExpressionNotPlusMinusNotName ::= NOT UnaryExpression
//
// void MakePreUnaryExpression();
//

//
// Rule 387:  UnaryExpressionNotPlusMinusNotName ::= CastExpression
//
// void NoAction();
//

//
// Rule 388:  CastExpression ::= LPAREN PrimitiveType Dimsopt RPAREN UnaryExpression
//
#line 5064 "java.g"
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

//
// Rule 389:  CastExpression ::= LPAREN Name Marker RPAREN UnaryExpressionNotPlusMinus
//
// void MakeCastExpression();
//

//
// Rule 390:  CastExpression ::= LPAREN Name Dims RPAREN UnaryExpressionNotPlusMinus
//
// void MakeCastExpression();
//

//
// Rule 391:  CastExpression ::= LPAREN Name LESS TypeArgumentList1 Dimsopt RPAREN UnaryExpressionNotPlusMinus
//
#line 5105 "java.g"
void Parser::Act391()
{
    Sym(4) = MakeTypeArguments(2);
    MakeCastExpression(MakeArrayType(4), 6);
}

//
// Rule 392:  CastExpression ::= LPAREN Name LESS TypeArgumentList1 DOT ClassOrInterfaceType Dimsopt RPAREN...
//
#line 5119 "java.g"
void Parser::Act392()
{
    AstTypeName* p = DYNAMIC_CAST<AstTypeName*> (Sym(6));
    while (p -> base_opt)
        p = p -> base_opt;
    p -> base_opt = MakeTypeArguments(2);
    MakeCastExpression(MakeArrayType(6), 8);
}

//
// Rule 393:  MultiplicativeExpression ::= UnaryExpression
//
// void NoAction();
//

//
// Rule 394:  MultiplicativeExpression ::= MultiplicativeExpression MULTIPLY UnaryExpression
//
#line 5136 "java.g"
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

//
// Rule 395:  MultiplicativeExpression ::= MultiplicativeExpression DIVIDE UnaryExpression
//
// void MakeBinaryExpression();
//

//
// Rule 396:  MultiplicativeExpression ::= MultiplicativeExpression REMAINDER UnaryExpression
//
// void MakeBinaryExpression();
//

//
// Rule 397:  MultiplicativeExpressionNotName ::= UnaryExpressionNotName
//
// void NoAction();
//

//
// Rule 398:  MultiplicativeExpressionNotName ::= MultiplicativeExpressionNotName MULTIPLY UnaryExpression
//
// void MakeBinaryExpression();
//

//
// Rule 399:  MultiplicativeExpressionNotName ::= Name MULTIPLY UnaryExpression
//
// void MakeBinaryExpression();
//

//
// Rule 400:  MultiplicativeExpressionNotName ::= MultiplicativeExpressionNotName DIVIDE UnaryExpression
//
// void MakeBinaryExpression();
//

//
// Rule 401:  MultiplicativeExpressionNotName ::= Name DIVIDE UnaryExpression
//
// void MakeBinaryExpression();
//

//
// Rule 402:  MultiplicativeExpressionNotName ::= MultiplicativeExpressionNotName REMAINDER UnaryExpression
//
// void MakeBinaryExpression();
//

//
// Rule 403:  MultiplicativeExpressionNotName ::= Name REMAINDER UnaryExpression
//
// void MakeBinaryExpression();
//

//
// Rule 404:  AdditiveExpression ::= MultiplicativeExpression
//
// void NoAction();
//

//
// Rule 405:  AdditiveExpression ::= AdditiveExpression PLUS MultiplicativeExpression
//
// void MakeBinaryExpression();
//

//
// Rule 406:  AdditiveExpression ::= AdditiveExpression MINUS MultiplicativeExpression
//
// void MakeBinaryExpression();
//

//
// Rule 407:  AdditiveExpressionNotName ::= MultiplicativeExpressionNotName
//
// void NoAction();
//

//
// Rule 408:  AdditiveExpressionNotName ::= AdditiveExpressionNotName PLUS MultiplicativeExpression
//
// void MakeBinaryExpression();
//

//
// Rule 409:  AdditiveExpressionNotName ::= Name PLUS MultiplicativeExpression
//
// void MakeBinaryExpression();
//

//
// Rule 410:  AdditiveExpressionNotName ::= AdditiveExpressionNotName MINUS MultiplicativeExpression
//
// void MakeBinaryExpression();
//

//
// Rule 411:  AdditiveExpressionNotName ::= Name MINUS MultiplicativeExpression
//
// void MakeBinaryExpression();
//

//
// Rule 412:  ShiftExpression ::= AdditiveExpression
//
// void NoAction();
//

//
// Rule 413:  ShiftExpression ::= ShiftExpression LEFT_SHIFT AdditiveExpression
//
// void MakeBinaryExpression();
//

//
// Rule 414:  ShiftExpression ::= ShiftExpression RIGHT_SHIFT AdditiveExpression
//
// void MakeBinaryExpression();
//

//
// Rule 415:  ShiftExpression ::= ShiftExpression UNSIGNED_RIGHT_SHIFT AdditiveExpression
//
// void MakeBinaryExpression();
//

//
// Rule 416:  ShiftExpressionNotName ::= AdditiveExpressionNotName
//
// void NoAction();
//

//
// Rule 417:  ShiftExpressionNotName ::= ShiftExpressionNotName LEFT_SHIFT AdditiveExpression
//
// void MakeBinaryExpression();
//

//
// Rule 418:  ShiftExpressionNotName ::= Name LEFT_SHIFT AdditiveExpression
//
// void MakeBinaryExpression();
//

//
// Rule 419:  ShiftExpressionNotName ::= ShiftExpressionNotName RIGHT_SHIFT AdditiveExpression
//
// void MakeBinaryExpression();
//

//
// Rule 420:  ShiftExpressionNotName ::= Name RIGHT_SHIFT AdditiveExpression
//
// void MakeBinaryExpression();
//

//
// Rule 421:  ShiftExpressionNotName ::= ShiftExpressionNotName UNSIGNED_RIGHT_SHIFT AdditiveExpression
//
// void MakeBinaryExpression();
//

//
// Rule 422:  ShiftExpressionNotName ::= Name UNSIGNED_RIGHT_SHIFT AdditiveExpression
//
// void MakeBinaryExpression();
//

//
// Rule 423:  RelationalExpression ::= ShiftExpression
//
// void NoAction();
//

//
// Rule 424:  RelationalExpression ::= ShiftExpression LESS ShiftExpression
//
// void MakeBinaryExpression();
//

//
// Rule 425:  RelationalExpression ::= RelationalExpression GREATER ShiftExpression
//
// void MakeBinaryExpression();
//

//
// Rule 426:  RelationalExpression ::= RelationalExpression LESS_EQUAL ShiftExpression
//
// void MakeBinaryExpression();
//

//
// Rule 427:  RelationalExpression ::= RelationalExpression GREATER_EQUAL ShiftExpression
//
// void MakeBinaryExpression();
//

//
// Rule 428:  RelationalExpression ::= RelationalExpression instanceof ReferenceType
//
#line 5320 "java.g"
void Parser::MakeInstanceofExpression()
{
    AstInstanceofExpression* p = ast_pool -> NewInstanceofExpression();
    p -> expression = DYNAMIC_CAST<AstExpression*> (Sym(1));
    p -> instanceof_token = Token(2);
    p -> type = DYNAMIC_CAST<AstType*> (Sym(3));
    Sym(1) = p;
}

//
// Rule 429:  RelationalExpressionNotName ::= ShiftExpressionNotName
//
// void NoAction();
//

//
// Rule 430:  RelationalExpressionNotName ::= ShiftExpressionNotName LESS ShiftExpression
//
// void MakeBinaryExpression();
//

//
// Rule 431:  RelationalExpressionNotName ::= Name LESS ShiftExpression
//
// void MakeBinaryExpression();
//

//
// Rule 432:  RelationalExpressionNotName ::= ShiftExpressionNotName GREATER ShiftExpression
//
// void MakeBinaryExpression();
//

//
// Rule 433:  RelationalExpressionNotName ::= Name GREATER ShiftExpression
//
// void MakeBinaryExpression();
//

//
// Rule 434:  RelationalExpressionNotName ::= RelationalExpressionNotName LESS_EQUAL ShiftExpression
//
// void MakeBinaryExpression();
//

//
// Rule 435:  RelationalExpressionNotName ::= Name LESS_EQUAL ShiftExpression
//
// void MakeBinaryExpression();
//

//
// Rule 436:  RelationalExpressionNotName ::= RelationalExpressionNotName GREATER_EQUAL ShiftExpression
//
// void MakeBinaryExpression();
//

//
// Rule 437:  RelationalExpressionNotName ::= Name GREATER_EQUAL ShiftExpression
//
// void MakeBinaryExpression();
//

//
// Rule 438:  RelationalExpressionNotName ::= RelationalExpressionNotName instanceof ReferenceType
//
// void MakeInstanceofExpression();
//

//
// Rule 439:  RelationalExpressionNotName ::= Name instanceof ReferenceType
//
// void MakeInstanceofExpression();
//

//
// Rule 440:  EqualityExpression ::= RelationalExpression
//
// void NoAction();
//

//
// Rule 441:  EqualityExpression ::= EqualityExpression EQUAL_EQUAL RelationalExpression
//
// void MakeBinaryExpression();
//

//
// Rule 442:  EqualityExpression ::= EqualityExpression NOT_EQUAL RelationalExpression
//
// void MakeBinaryExpression();
//

//
// Rule 443:  EqualityExpressionNotName ::= RelationalExpressionNotName
//
// void NoAction();
//

//
// Rule 444:  EqualityExpressionNotName ::= EqualityExpressionNotName EQUAL_EQUAL RelationalExpression
//
// void MakeBinaryExpression();
//

//
// Rule 445:  EqualityExpressionNotName ::= Name EQUAL_EQUAL RelationalExpression
//
// void MakeBinaryExpression();
//

//
// Rule 446:  EqualityExpressionNotName ::= EqualityExpressionNotName NOT_EQUAL RelationalExpression
//
// void MakeBinaryExpression();
//

//
// Rule 447:  EqualityExpressionNotName ::= Name NOT_EQUAL RelationalExpression
//
// void MakeBinaryExpression();
//

//
// Rule 448:  AndExpression ::= EqualityExpression
//
// void NoAction();
//

//
// Rule 449:  AndExpression ::= AndExpression AND EqualityExpression
//
// void MakeBinaryExpression();
//

//
// Rule 450:  AndExpressionNotName ::= EqualityExpressionNotName
//
// void NoAction();
//

//
// Rule 451:  AndExpressionNotName ::= AndExpressionNotName AND EqualityExpression
//
// void MakeBinaryExpression();
//

//
// Rule 452:  AndExpressionNotName ::= Name AND EqualityExpression
//
// void MakeBinaryExpression();
//

//
// Rule 453:  ExclusiveOrExpression ::= AndExpression
//
// void NoAction();
//

//
// Rule 454:  ExclusiveOrExpression ::= ExclusiveOrExpression XOR AndExpression
//
// void MakeBinaryExpression();
//

//
// Rule 455:  ExclusiveOrExpressionNotName ::= AndExpressionNotName
//
// void NoAction();
//

//
// Rule 456:  ExclusiveOrExpressionNotName ::= ExclusiveOrExpressionNotName XOR AndExpression
//
// void MakeBinaryExpression();
//

//
// Rule 457:  ExclusiveOrExpressionNotName ::= Name XOR AndExpression
//
// void MakeBinaryExpression();
//

//
// Rule 458:  InclusiveOrExpression ::= ExclusiveOrExpression
//
// void NoAction();
//

//
// Rule 459:  InclusiveOrExpression ::= InclusiveOrExpression OR ExclusiveOrExpression
//
// void MakeBinaryExpression();
//

//
// Rule 460:  InclusiveOrExpressionNotName ::= ExclusiveOrExpressionNotName
//
// void NoAction();
//

//
// Rule 461:  InclusiveOrExpressionNotName ::= InclusiveOrExpressionNotName OR ExclusiveOrExpression
//
// void MakeBinaryExpression();
//

//
// Rule 462:  InclusiveOrExpressionNotName ::= Name OR ExclusiveOrExpression
//
// void MakeBinaryExpression();
//

//
// Rule 463:  ConditionalAndExpression ::= InclusiveOrExpression
//
// void NoAction();
//

//
// Rule 464:  ConditionalAndExpression ::= ConditionalAndExpression AND_AND InclusiveOrExpression
//
// void MakeBinaryExpression();
//

//
// Rule 465:  ConditionalAndExpressionNotName ::= InclusiveOrExpressionNotName
//
// void NoAction();
//

//
// Rule 466:  ConditionalAndExpressionNotName ::= ConditionalAndExpressionNotName AND_AND InclusiveOrExpression
//
// void MakeBinaryExpression();
//

//
// Rule 467:  ConditionalAndExpressionNotName ::= Name AND_AND InclusiveOrExpression
//
// void MakeBinaryExpression();
//

//
// Rule 468:  ConditionalOrExpression ::= ConditionalAndExpression
//
// void NoAction();
//

//
// Rule 469:  ConditionalOrExpression ::= ConditionalOrExpression OR_OR ConditionalAndExpression
//
// void MakeBinaryExpression();
//

//
// Rule 470:  ConditionalOrExpressionNotName ::= ConditionalAndExpressionNotName
//
// void NoAction();
//

//
// Rule 471:  ConditionalOrExpressionNotName ::= ConditionalOrExpressionNotName OR_OR ConditionalAndExpression
//
// void MakeBinaryExpression();
//

//
// Rule 472:  ConditionalOrExpressionNotName ::= Name OR_OR ConditionalAndExpression
//
// void MakeBinaryExpression();
//

//
// Rule 473:  ConditionalExpression ::= ConditionalOrExpression
//
// void NoAction();
//

//
// Rule 474:  ConditionalExpression ::= ConditionalOrExpression QUESTION Expression COLON ConditionalExpression
//
#line 5534 "java.g"
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

//
// Rule 475:  ConditionalExpressionNotName ::= ConditionalOrExpressionNotName
//
// void NoAction();
//

//
// Rule 476:  ConditionalExpressionNotName ::= ConditionalOrExpressionNotName QUESTION Expression COLON...
//
// void MakeConditionalExpression();
//

//
// Rule 477:  ConditionalExpressionNotName ::= Name QUESTION Expression COLON ConditionalExpression
//
// void MakeConditionalExpression();
//

//
// Rule 478:  AssignmentExpression ::= ConditionalExpression
//
// void NoAction();
//

//
// Rule 479:  AssignmentExpression ::= Assignment
//
// void NoAction();
//

//
// Rule 480:  AssignmentExpressionNotName ::= ConditionalExpressionNotName
//
// void NoAction();
//

//
// Rule 481:  AssignmentExpressionNotName ::= Assignment
//
// void NoAction();
//

//
// Rule 482:  Assignment ::= PostfixExpression AssignmentOperator AssignmentExpression
//
#line 5591 "java.g"
void Parser::Act482()
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

//
// Rule 483:  AssignmentOperator ::= EQUAL
//
// void NoAction();
//

//
// Rule 484:  AssignmentOperator ::= MULTIPLY_EQUAL
//
// void NoAction();
//

//
// Rule 485:  AssignmentOperator ::= DIVIDE_EQUAL
//
// void NoAction();
//

//
// Rule 486:  AssignmentOperator ::= REMAINDER_EQUAL
//
// void NoAction();
//

//
// Rule 487:  AssignmentOperator ::= PLUS_EQUAL
//
// void NoAction();
//

//
// Rule 488:  AssignmentOperator ::= MINUS_EQUAL
//
// void NoAction();
//

//
// Rule 489:  AssignmentOperator ::= LEFT_SHIFT_EQUAL
//
// void NoAction();
//

//
// Rule 490:  AssignmentOperator ::= RIGHT_SHIFT_EQUAL
//
// void NoAction();
//

//
// Rule 491:  AssignmentOperator ::= UNSIGNED_RIGHT_SHIFT_EQUAL
//
// void NoAction();
//

//
// Rule 492:  AssignmentOperator ::= AND_EQUAL
//
// void NoAction();
//

//
// Rule 493:  AssignmentOperator ::= XOR_EQUAL
//
// void NoAction();
//

//
// Rule 494:  AssignmentOperator ::= OR_EQUAL
//
// void NoAction();
//

//
// Rule 495:  Expression ::= AssignmentExpression
//
// void NoAction();
//

//
// Rule 496:  Expressionopt ::=
//
// void NullAction();
//

//
// Rule 497:  Expressionopt ::= Expression
//
// void NoAction();
//

//
// Rule 498:  ExpressionNotName ::= AssignmentExpressionNotName
//
// void NoAction();
//

//
// Rule 499:  Marker ::=
//
// void NullAction();
//

//
// Rule 500:  ,opt ::=
//
// void NoAction();
//

//
// Rule 501:  ,opt ::= COMMA
//
// void NoAction();
//

//
// Rule 502:  Identifieropt ::=
//
// void NoAction();
//

//
// Rule 503:  Identifieropt ::= Identifier
//
// void NoAction();
//

//
// Rule 504:  PackageHeaderMarker ::=
//
#line 5730 "java.g"
//
// When this function is invoked, if the "parse_package_header_only" flag
// is turned on, we skip to the end-of-file token.
//
void Parser::Act504()
{
    if (parse_package_header_only)
        // point to the EOF token
        lex_stream -> Reset(lex_stream -> NumTokens() - 1);
    Sym(1) = NULL;
}

//
// Rule 505:  MethodHeaderMarker ::=
//
#line 5746 "java.g"
//
// When this function is invoked, if the "parse_header_only" flag
// is turned on, the body of the method being parsed is skipped.
//
void Parser::Act505()
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

//
// Rule 506:  TypeArguments ::= LESS TypeArgumentList1
//
// void SetSym1ToSym2();
//

//
// Rule 507:  TypeArgumentsopt ::=
//
// void NullAction();
//

//
// Rule 508:  TypeArgumentsopt ::= TypeArguments
//
// void NoAction();
//

//
// Rule 509:  Wildcard ::= QUESTION Marker Marker Marker
//
#line 5809 "java.g"
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

//
// Rule 510:  Wildcard ::= QUESTION extends Marker ReferenceType
//
// void MakeWildcard();
//

//
// Rule 511:  Wildcard ::= QUESTION Marker super ReferenceType
//
// void MakeWildcard();
//

//
// Rule 512:  Wildcard1 ::= QUESTION Marker Marker GREATER
//
// void MakeWildcard();
//

//
// Rule 513:  Wildcard1 ::= QUESTION extends Marker ReferenceType1
//
// void MakeWildcard();
//

//
// Rule 514:  Wildcard1 ::= QUESTION Marker super ReferenceType1
//
// void MakeWildcard();
//

//
// Rule 515:  Wildcard2 ::= QUESTION Marker Marker RIGHT_SHIFT
//
// void MakeWildcard();
//

//
// Rule 516:  Wildcard2 ::= QUESTION extends Marker ReferenceType2
//
// void MakeWildcard();
//

//
// Rule 517:  Wildcard2 ::= QUESTION Marker super ReferenceType2
//
// void MakeWildcard();
//

//
// Rule 518:  Wildcard3 ::= QUESTION Marker Marker UNSIGNED_RIGHT_SHIFT
//
// void MakeWildcard();
//

//
// Rule 519:  Wildcard3 ::= QUESTION extends Marker ReferenceType3
//
// void MakeWildcard();
//

//
// Rule 520:  Wildcard3 ::= QUESTION Marker super ReferenceType3
//
// void MakeWildcard();
//

//
// Rule 521:  TypeArgumentList ::= TypeArgument
//
// void StartList();
//

//
// Rule 522:  TypeArgumentList ::= TypeArgumentList COMMA TypeArgument
//
// void AddList3();
//

//
// Rule 523:  TypeArgumentList1 ::= TypeArgument1
//
// void StartList();
//

//
// Rule 524:  TypeArgumentList1 ::= TypeArgumentList COMMA TypeArgument1
//
// void AddList3();
//

//
// Rule 525:  TypeArgumentList2 ::= TypeArgument2
//
// void StartList();
//

//
// Rule 526:  TypeArgumentList2 ::= TypeArgumentList COMMA TypeArgument2
//
// void AddList3();
//

//
// Rule 527:  TypeArgumentList3 ::= TypeArgument3
//
// void StartList();
//

//
// Rule 528:  TypeArgumentList3 ::= TypeArgumentList COMMA TypeArgument3
//
// void AddList3();
//

//
// Rule 529:  TypeArgument ::= ReferenceType
//
// void NoAction();
//

//
// Rule 530:  TypeArgument ::= Wildcard
//
// void NoAction();
//

//
// Rule 531:  TypeArgument1 ::= ReferenceType1
//
// void NoAction();
//

//
// Rule 532:  TypeArgument1 ::= Wildcard1
//
// void NoAction();
//

//
// Rule 533:  TypeArgument2 ::= ReferenceType2
//
// void NoAction();
//

//
// Rule 534:  TypeArgument2 ::= Wildcard2
//
// void NoAction();
//

//
// Rule 535:  TypeArgument3 ::= ReferenceType3
//
// void NoAction();
//

//
// Rule 536:  TypeArgument3 ::= Wildcard3
//
// void NoAction();
//

//
// Rule 537:  ReferenceType1 ::= ReferenceType GREATER
//
// void NoAction();
//

//
// Rule 538:  ReferenceType1 ::= ClassOrInterface LESS TypeArgumentList2 Marker
//
// void MakeTypeArguments();
//

//
// Rule 539:  ReferenceType2 ::= ReferenceType RIGHT_SHIFT
//
// void NoAction();
//

//
// Rule 540:  ReferenceType2 ::= ClassOrInterface LESS TypeArgumentList3 Marker
//
// void MakeTypeArguments();
//

//
// Rule 541:  ReferenceType3 ::= ReferenceType UNSIGNED_RIGHT_SHIFT
//
// void NoAction();
//

//
// Rule 542:  TypeParameters ::= LESS TypeParameterList1
//
// void SetSym1ToSym2();
//

//
// Rule 543:  TypeParametersopt ::=
//
// void NullAction();
//

//
// Rule 544:  TypeParametersopt ::= TypeParameters
//
// void NoAction();
//

//
// Rule 545:  TypeParameterList ::= TypeParameter
//
// void StartList();
//

//
// Rule 546:  TypeParameterList ::= TypeParameterList COMMA TypeParameter
//
// void AddList3();
//

//
// Rule 547:  TypeParameterList1 ::= TypeParameter1
//
// void StartList();
//

//
// Rule 548:  TypeParameterList1 ::= TypeParameterList COMMA TypeParameter1
//
// void AddList3();
//

//
// Rule 549:  TypeParameter ::= Identifier TypeBoundopt
//
#line 6160 "java.g"
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

//
// Rule 550:  TypeParameter1 ::= Identifier Marker GREATER
//
// void MakeTypeParameter();
//

//
// Rule 551:  TypeParameter1 ::= Identifier TypeBound1
//
// void MakeTypeParameter();
//

//
// Rule 552:  TypeBound ::= extends ReferenceType AdditionalBoundListopt
//
#line 6210 "java.g"
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

//
// Rule 553:  TypeBoundopt ::=
//
// void NullAction();
//

//
// Rule 554:  TypeBoundopt ::= TypeBound
//
// void NoAction();
//

//
// Rule 555:  TypeBound1 ::= extends ReferenceType1 Marker
//
// void MakeTypeBound();
//

//
// Rule 556:  TypeBound1 ::= extends ReferenceType AdditionalBoundList1
//
// void MakeTypeBound();
//

//
// Rule 557:  AdditionalBoundList ::= AdditionalBound
//
// void StartList();
//

//
// Rule 558:  AdditionalBoundList ::= AdditionalBoundList AdditionalBound
//
// void AddList2();
//

//
// Rule 559:  AdditionalBoundListopt ::=
//
// void NullAction();
//

//
// Rule 560:  AdditionalBoundListopt ::= AdditionalBoundList
//
// void NoAction();
//

//
// Rule 561:  AdditionalBoundList1 ::= AdditionalBound1
//
// void StartList();
//

//
// Rule 562:  AdditionalBoundList1 ::= AdditionalBoundList AdditionalBound1
//
// void AddList2();
//

//
// Rule 563:  AdditionalBound ::= AND ClassOrInterfaceType
//
// void SetSym1ToSym2();
//

//
// Rule 564:  AdditionalBound1 ::= AND ClassOrInterfaceType1
//
// void SetSym1ToSym2();
//

//
// Rule 565:  ClassOrInterfaceType1 ::= ClassOrInterfaceType GREATER
//
// void NoAction();
//

//
// Rule 566:  ClassOrInterfaceType1 ::= ClassOrInterface LESS TypeArgumentList2 Marker
//
// void MakeTypeArguments();
//

} // Close namespace Jopa block
