-- Java 7 Grammar for jikespg
-- Complete grammar extracted from original java.g
-- 566 rules total

%Options act
%Options table
%Options gp=cpp
%Options fp=java
%Options prefix=TK_
%Options error-maps
%Options scopes
%Options names=max

%Terminals
    Identifier
    IntegerLiteral
    LongLiteral
    FloatLiteral
    DoubleLiteral
    CharacterLiteral
    StringLiteral
    BodyMarker
    ERROR
    EOF
    PLUS_PLUS
    MINUS_MINUS
    EQUAL_EQUAL
    LESS_EQUAL
    GREATER_EQUAL
    NOT_EQUAL
    LEFT_SHIFT
    RIGHT_SHIFT
    UNSIGNED_RIGHT_SHIFT
    PLUS_EQUAL
    MINUS_EQUAL
    MULTIPLY_EQUAL
    DIVIDE_EQUAL
    AND_EQUAL
    OR_EQUAL
    XOR_EQUAL
    REMAINDER_EQUAL
    LEFT_SHIFT_EQUAL
    RIGHT_SHIFT_EQUAL
    UNSIGNED_RIGHT_SHIFT_EQUAL
    OR_OR
    AND_AND
    PLUS
    MINUS
    NOT
    REMAINDER
    XOR
    AND
    MULTIPLY
    OR
    TWIDDLE
    DIVIDE
    GREATER
    LESS
    LPAREN
    RPAREN
    LBRACE
    RBRACE
    LBRACKET
    RBRACKET
    SEMICOLON
    QUESTION
    COLON
    COMMA
    DOT
    EQUAL
    AT
    ELLIPSIS
    abstract
    assert
    boolean
    break
    byte
    case
    catch
    char
    class
    continue
    default
    do
    double
    else
    enum
    extends
    false
    final
    finally
    float
    for
    if
    implements
    import
    instanceof
    int
    interface
    long
    native
    new
    null
    package
    private
    protected
    public
    return
    short
    static
    strictfp
    super
    switch
    synchronized
    this
    throw
    throws
    transient
    true
    try
    void
    volatile
    while
    const
    goto

%Alias
    '++' ::= PLUS_PLUS
    '--' ::= MINUS_MINUS
    '==' ::= EQUAL_EQUAL
    '<=' ::= LESS_EQUAL
    '>=' ::= GREATER_EQUAL
    '!=' ::= NOT_EQUAL
    '<<' ::= LEFT_SHIFT
    '>>' ::= RIGHT_SHIFT
    '>>>' ::= UNSIGNED_RIGHT_SHIFT
    '+=' ::= PLUS_EQUAL
    '-=' ::= MINUS_EQUAL
    '*=' ::= MULTIPLY_EQUAL
    '/=' ::= DIVIDE_EQUAL
    '&=' ::= AND_EQUAL
    '|=' ::= OR_EQUAL
    '^=' ::= XOR_EQUAL
    '%=' ::= REMAINDER_EQUAL
    '<<=' ::= LEFT_SHIFT_EQUAL
    '>>=' ::= RIGHT_SHIFT_EQUAL
    '>>>=' ::= UNSIGNED_RIGHT_SHIFT_EQUAL
    '||' ::= OR_OR
    '&&' ::= AND_AND
    '+' ::= PLUS
    '-' ::= MINUS
    '!' ::= NOT
    '%' ::= REMAINDER
    '^' ::= XOR
    '&' ::= AND
    '*' ::= MULTIPLY
    '|' ::= OR
    '~' ::= TWIDDLE
    '/' ::= DIVIDE
    '>' ::= GREATER
    '<' ::= LESS
    '(' ::= LPAREN
    ')' ::= RPAREN
    '{' ::= LBRACE
    '}' ::= RBRACE
    '[' ::= LBRACKET
    ']' ::= RBRACKET
    ';' ::= SEMICOLON
    '?' ::= QUESTION
    ':' ::= COLON
    ',' ::= COMMA
    '.' ::= DOT
    '=' ::= EQUAL
    '@' ::= AT
    '...' ::= ELLIPSIS
    %EOF ::= EOF
    %ERROR ::= ERROR

%Rules

Goal ::= CompilationUnit
Goal ::= BodyMarker MethodBody
Literal ::= IntegerLiteral
Literal ::= LongLiteral
Literal ::= FloatLiteral
Literal ::= DoubleLiteral
Literal ::= BooleanLiteral
Literal ::= CharacterLiteral
Literal ::= StringLiteral
Literal ::= 'null'
BooleanLiteral ::= 'true'
BooleanLiteral ::= 'false'
Type ::= PrimitiveType
Type ::= ReferenceType
PrimitiveType ::= NumericType
PrimitiveType ::= 'boolean'
NumericType ::= IntegralType
NumericType ::= FloatingPointType
IntegralType ::= 'byte'
IntegralType ::= 'short'
IntegralType ::= 'int'
IntegralType ::= 'long'
IntegralType ::= 'char'
FloatingPointType ::= 'float'
FloatingPointType ::= 'double'
VoidType ::= 'void'
ReferenceType ::= ClassOrInterfaceType
ReferenceType ::= ArrayType
ClassOrInterfaceType ::= ClassOrInterface
ClassOrInterfaceType ::= ClassOrInterface '<' TypeArgumentList1 Marker
ClassOrInterface ::= Name
ClassOrInterface ::= ClassOrInterface '<' TypeArgumentList1 '.' Name
ArrayType ::= PrimitiveType Dims
ArrayType ::= Name Dims
ArrayType ::= ClassOrInterface '<' TypeArgumentList1 '.' Name Dims
ArrayType ::= ClassOrInterface '<' TypeArgumentList1 Dims
Name ::= 'Identifier'
Name ::= Name '.' Marker 'Identifier'
CompilationUnit ::= PackageDeclaration ImportDeclarationsopt TypeDeclarationsopt
CompilationUnit ::= Marker ImportDeclarations TypeDeclarationsopt
CompilationUnit ::= TypeDeclarationsopt
ImportDeclarations ::= ImportDeclaration
ImportDeclarations ::= ImportDeclarations ImportDeclaration
ImportDeclarationsopt ::= %empty
ImportDeclarationsopt ::= ImportDeclarations
TypeDeclarations ::= TypeDeclaration
TypeDeclarations ::= TypeDeclarations TypeDeclaration
TypeDeclarationsopt ::= %empty
TypeDeclarationsopt ::= TypeDeclarations
PackageDeclaration ::= Marker 'package' Name PackageHeaderMarker ';'
PackageDeclaration ::= Modifiers 'package' Name PackageHeaderMarker ';'
ImportDeclaration ::= SingleTypeImportDeclaration
ImportDeclaration ::= TypeImportOnDemandDeclaration
ImportDeclaration ::= SingleStaticImportDeclaration
ImportDeclaration ::= StaticImportOnDemandDeclaration
SingleTypeImportDeclaration ::= 'import' Marker Name Marker Marker ';'
TypeImportOnDemandDeclaration ::= 'import' Marker Name '.' '*' ';'
SingleStaticImportDeclaration ::= 'import' 'static' Name Marker Marker ';'
StaticImportOnDemandDeclaration ::= 'import' 'static' Name '.' '*' ';'
TypeDeclaration ::= ClassDeclaration
TypeDeclaration ::= EnumDeclaration
TypeDeclaration ::= InterfaceDeclaration
TypeDeclaration ::= AnnotationTypeDeclaration
TypeDeclaration ::= ';'
Modifiers ::= Modifier
Modifiers ::= Modifiers Modifier
Modifiersopt ::= %empty
Modifiersopt ::= Modifiers
Modifier ::= 'public'
Modifier ::= 'protected'
Modifier ::= 'private'
Modifier ::= 'static'
Modifier ::= 'abstract'
Modifier ::= 'final'
Modifier ::= 'native'
Modifier ::= 'strictfp'
Modifier ::= 'synchronized'
Modifier ::= 'transient'
Modifier ::= 'volatile'
Modifier ::= Annotation
Annotation ::= NormalAnnotation
Annotation ::= MarkerAnnotation
Annotation ::= SingleMemberAnnotation
NormalAnnotation ::= '@' Name '(' MemberValuePairsopt ')'
MemberValuePairs ::= MemberValuePair
MemberValuePairs ::= MemberValuePairs ',' MemberValuePair
MemberValuePairsopt ::= %empty
MemberValuePairsopt ::= MemberValuePairs
MemberValuePair ::= 'Identifier' '=' MemberValue
MemberValue ::= ConditionalExpression
MemberValue ::= Annotation
MemberValue ::= MemberValueArrayInitializer
MemberValueArrayInitializer ::= '{' Marker ,opt '}'
MemberValueArrayInitializer ::= '{' MemberValues ,opt '}'
MemberValues ::= MemberValue
MemberValues ::= MemberValues ',' MemberValue
MarkerAnnotation ::= '@' Name Marker Marker Marker
SingleMemberAnnotation ::= '@' Name '(' MemberValue ')'
ClassDeclaration ::= Marker 'class' 'Identifier' TypeParametersopt Superopt Interfacesopt ClassBody
ClassDeclaration ::= Modifiers 'class' 'Identifier' TypeParametersopt Superopt Interfacesopt ClassBody
Super ::= 'extends' ClassOrInterfaceType
Superopt ::= %empty
Superopt ::= Super
Interfaces ::= 'implements' TypeList
Interfacesopt ::= %empty
Interfacesopt ::= Interfaces
TypeList ::= ClassOrInterfaceType
TypeList ::= TypeList ',' ClassOrInterfaceType
ClassBody ::= '{' ClassBodyDeclarationsopt '}'
ClassBodyopt ::= %empty
ClassBodyopt ::= ClassBody
ClassBodyDeclarations ::= ClassBodyDeclaration
ClassBodyDeclarations ::= ClassBodyDeclarations ClassBodyDeclaration
ClassBodyDeclarationsopt ::= %empty
ClassBodyDeclarationsopt ::= ClassBodyDeclarations
ClassBodyDeclaration ::= ConstructorDeclaration
ClassBodyDeclaration ::= InitializerDeclaration
ClassBodyDeclaration ::= FieldDeclaration
ClassBodyDeclaration ::= MethodDeclaration
ClassBodyDeclaration ::= TypeDeclaration
FieldDeclaration ::= Marker Marker Type VariableDeclarators ';'
FieldDeclaration ::= Modifiers Marker Type VariableDeclarators ';'
VariableDeclarators ::= VariableDeclarator
VariableDeclarators ::= VariableDeclarators ',' VariableDeclarator
VariableDeclarator ::= VariableDeclaratorId
VariableDeclarator ::= VariableDeclaratorId '=' VariableInitializer
VariableDeclaratorId ::= 'Identifier' Dimsopt
VariableInitializer ::= Expression
VariableInitializer ::= ArrayInitializer
MethodDeclaration ::= MethodHeader MethodHeaderMarker MethodBody
MethodDeclaration ::= MethodHeader MethodHeaderMarker Marker ';'
MethodHeader ::= Marker Marker Type MethodDeclarator Throwsopt
MethodHeader ::= Modifiers Marker Type MethodDeclarator Throwsopt
MethodHeader ::= Marker TypeParameters Type MethodDeclarator Throwsopt
MethodHeader ::= Modifiers TypeParameters Type MethodDeclarator Throwsopt
MethodHeader ::= Marker Marker VoidType MethodDeclarator Throwsopt
MethodHeader ::= Modifiers Marker VoidType MethodDeclarator Throwsopt
MethodHeader ::= Marker TypeParameters VoidType MethodDeclarator Throwsopt
MethodHeader ::= Modifiers TypeParameters VoidType MethodDeclarator Throwsopt
MethodDeclarator ::= 'Identifier' '(' FormalParameterListopt ')' Dimsopt
FormalParameterList ::= LastFormalParameter
FormalParameterList ::= FormalParameters ',' LastFormalParameter
FormalParameterListopt ::= %empty
FormalParameterListopt ::= FormalParameterList
FormalParameters ::= FormalParameter
FormalParameters ::= FormalParameters ',' FormalParameter
FormalParameter ::= Type Marker Marker VariableDeclaratorId
FormalParameter ::= Modifiers Type Marker VariableDeclaratorId
LastFormalParameter ::= FormalParameter
LastFormalParameter ::= Type Marker '...' VariableDeclaratorId
LastFormalParameter ::= Modifiers Type '...' VariableDeclaratorId
Throws ::= 'throws' TypeList
Throwsopt ::= %empty
Throwsopt ::= Throws
MethodBody ::= '{' BlockStatementsopt '}'
InitializerDeclaration ::= Marker MethodHeaderMarker MethodBody
InitializerDeclaration ::= Modifiers MethodHeaderMarker MethodBody
ConstructorDeclaration ::= Marker Marker ConstructorDeclarator Throwsopt MethodHeaderMarker MethodBody
ConstructorDeclaration ::= Modifiers Marker ConstructorDeclarator Throwsopt MethodHeaderMarker MethodBody
ConstructorDeclaration ::= Marker TypeParameters ConstructorDeclarator Throwsopt MethodHeaderMarker MethodBody
ConstructorDeclaration ::= Modifiers TypeParameters ConstructorDeclarator Throwsopt MethodHeaderMarker MethodBody
ConstructorDeclarator ::= 'Identifier' '(' FormalParameterListopt ')' Marker
ExplicitConstructorInvocation ::= 'this' Arguments ';'
ExplicitConstructorInvocation ::= TypeArguments 'this' Arguments ';'
ExplicitConstructorInvocation ::= 'super' Arguments ';'
ExplicitConstructorInvocation ::= TypeArguments 'super' Arguments ';'
ExplicitConstructorInvocation ::= Primary '.' TypeArgumentsopt 'super' Arguments ';'
ExplicitConstructorInvocation ::= Name '.' Marker 'super' Arguments ';'
ExplicitConstructorInvocation ::= Name '.' TypeArguments 'super' Arguments ';'
EnumDeclaration ::= Marker 'enum' 'Identifier' Interfacesopt EnumBody
EnumDeclaration ::= Modifiers 'enum' 'Identifier' Interfacesopt EnumBody
EnumBody ::= '{' Marker ,opt EnumBodyDeclarationsopt '}'
EnumBody ::= '{' EnumConstants ,opt EnumBodyDeclarationsopt '}'
EnumConstants ::= EnumConstant
EnumConstants ::= EnumConstants ',' EnumConstant
EnumConstant ::= Modifiersopt 'Identifier' Argumentsopt ClassBodyopt
Arguments ::= '(' ArgumentListopt ')'
Argumentsopt ::= %empty
Argumentsopt ::= Arguments
EnumBodyDeclarations ::= ';' ClassBodyDeclarationsopt Marker
EnumBodyDeclarationsopt ::= %empty
EnumBodyDeclarationsopt ::= EnumBodyDeclarations
InterfaceDeclaration ::= Marker 'interface' 'Identifier' TypeParametersopt ExtendsInterfacesopt InterfaceBody
InterfaceDeclaration ::= Modifiers 'interface' 'Identifier' TypeParametersopt ExtendsInterfacesopt InterfaceBody
ExtendsInterfaces ::= 'extends' TypeList
ExtendsInterfacesopt ::= %empty
ExtendsInterfacesopt ::= ExtendsInterfaces
InterfaceBody ::= '{' InterfaceMemberDeclarationsopt '}'
InterfaceMemberDeclarations ::= InterfaceMemberDeclaration
InterfaceMemberDeclarations ::= InterfaceMemberDeclarations InterfaceMemberDeclaration
InterfaceMemberDeclarationsopt ::= %empty
InterfaceMemberDeclarationsopt ::= InterfaceMemberDeclarations
InterfaceMemberDeclaration ::= ConstantDeclaration
InterfaceMemberDeclaration ::= TypeDeclaration
ConstantDeclaration ::= FieldDeclaration
InterfaceMemberDeclaration ::= MethodDeclaration
AnnotationTypeDeclaration ::= '@' Marker 'interface' 'Identifier' AnnotationTypeBody
AnnotationTypeDeclaration ::= Modifiers '@' 'interface' 'Identifier' AnnotationTypeBody
AnnotationTypeBody ::= '{' AnnotationTypeMemberDeclarationsopt '}'
AnnotationTypeMemberDeclarations ::= AnnotationTypeMemberDeclaration
AnnotationTypeMemberDeclarations ::= AnnotationTypeMemberDeclarations AnnotationTypeMemberDeclaration
AnnotationTypeMemberDeclarationsopt ::= %empty
AnnotationTypeMemberDeclarationsopt ::= AnnotationTypeMemberDeclarations
AnnotationTypeMemberDeclaration ::= Marker Marker Type 'Identifier' '(' ')' DefaultValueopt ';'
AnnotationTypeMemberDeclaration ::= Modifiers Marker Type 'Identifier' '(' ')' DefaultValueopt ';'
AnnotationTypeMemberDeclaration ::= ConstantDeclaration
AnnotationTypeMemberDeclaration ::= TypeDeclaration
DefaultValue ::= 'default' MemberValue
DefaultValueopt ::= %empty
DefaultValueopt ::= DefaultValue
ArrayInitializer ::= '{' Marker ,opt '}'
ArrayInitializer ::= '{' VariableInitializers ,opt '}'
VariableInitializers ::= VariableInitializer
VariableInitializers ::= VariableInitializers ',' VariableInitializer
Block ::= '{' BlockStatementsopt '}'
BlockStatements ::= BlockStatement
BlockStatements ::= BlockStatements BlockStatement
BlockStatementsopt ::= %empty
BlockStatementsopt ::= BlockStatements
BlockStatement ::= LocalVariableDeclarationStatement
BlockStatement ::= Statement
BlockStatement ::= ClassDeclaration
BlockStatement ::= EnumDeclaration
BlockStatement ::= ExplicitConstructorInvocation
LocalVariableDeclarationStatement ::= LocalVariableDeclaration ';'
LocalVariableDeclaration ::= Type Marker Marker VariableDeclarators
LocalVariableDeclaration ::= Modifiers Type Marker VariableDeclarators
Statement ::= StatementWithoutTrailingSubstatement
Statement ::= LabeledStatement
Statement ::= IfThenStatement
Statement ::= IfThenElseStatement
Statement ::= WhileStatement
Statement ::= ForStatement
Statement ::= ForeachStatement
StatementNoShortIf ::= StatementWithoutTrailingSubstatement
StatementNoShortIf ::= LabeledStatementNoShortIf
StatementNoShortIf ::= IfThenElseStatementNoShortIf
StatementNoShortIf ::= WhileStatementNoShortIf
StatementNoShortIf ::= ForStatementNoShortIf
StatementNoShortIf ::= ForeachStatementNoShortIf
StatementWithoutTrailingSubstatement ::= Block
StatementWithoutTrailingSubstatement ::= EmptyStatement
StatementWithoutTrailingSubstatement ::= ExpressionStatement
StatementWithoutTrailingSubstatement ::= SwitchStatement
StatementWithoutTrailingSubstatement ::= DoStatement
StatementWithoutTrailingSubstatement ::= BreakStatement
StatementWithoutTrailingSubstatement ::= ContinueStatement
StatementWithoutTrailingSubstatement ::= ReturnStatement
StatementWithoutTrailingSubstatement ::= SynchronizedStatement
StatementWithoutTrailingSubstatement ::= ThrowStatement
StatementWithoutTrailingSubstatement ::= TryStatement
StatementWithoutTrailingSubstatement ::= AssertStatement
EmptyStatement ::= ';'
LabeledStatement ::= 'Identifier' ':' Statement
LabeledStatementNoShortIf ::= 'Identifier' ':' StatementNoShortIf
ExpressionStatement ::= StatementExpression ';'
StatementExpression ::= Assignment
StatementExpression ::= PreIncrementExpression
StatementExpression ::= PreDecrementExpression
StatementExpression ::= PostIncrementExpression
StatementExpression ::= PostDecrementExpression
StatementExpression ::= MethodInvocation
StatementExpression ::= ClassInstanceCreationExpression
IfThenStatement ::= 'if' '(' Expression ')' Statement Marker Marker
IfThenElseStatement ::= 'if' '(' Expression ')' StatementNoShortIf 'else' Statement
IfThenElseStatementNoShortIf ::= 'if' '(' Expression ')' StatementNoShortIf 'else' StatementNoShortIf
SwitchStatement ::= 'switch' '(' Expression ')' SwitchBlock
SwitchBlock ::= '{' SwitchBlockStatements SwitchLabelsopt '}'
SwitchBlock ::= '{' SwitchLabelsopt '}'
SwitchBlockStatements ::= SwitchBlockStatement
SwitchBlockStatements ::= SwitchBlockStatements SwitchBlockStatement
SwitchBlockStatement ::= SwitchLabels BlockStatements
SwitchLabels ::= SwitchLabel
SwitchLabels ::= SwitchLabels SwitchLabel
SwitchLabelsopt ::= %empty
SwitchLabelsopt ::= SwitchLabels
SwitchLabel ::= 'case' Expression ':'
SwitchLabel ::= 'default' Marker ':'
WhileStatement ::= 'while' '(' Expression ')' Statement
WhileStatementNoShortIf ::= 'while' '(' Expression ')' StatementNoShortIf
DoStatement ::= 'do' Statement 'while' '(' Expression ')' ';'
ForStatement ::= 'for' '(' ForInitopt ';' Expressionopt ';' ForUpdateopt ')' Statement
ForStatementNoShortIf ::= 'for' '(' ForInitopt ';' Expressionopt ';' ForUpdateopt ')' StatementNoShortIf
ForInit ::= StatementExpressionList
ForInit ::= LocalVariableDeclaration
ForInitopt ::= %empty
ForInitopt ::= ForInit
ForUpdate ::= StatementExpressionList
ForUpdateopt ::= %empty
ForUpdateopt ::= ForUpdate
StatementExpressionList ::= StatementExpression
StatementExpressionList ::= StatementExpressionList ',' StatementExpression
ForeachStatement ::= 'for' '(' FormalParameter ':' Expression ')' Statement
ForeachStatementNoShortIf ::= 'for' '(' FormalParameter ':' Expression ')' StatementNoShortIf
AssertStatement ::= 'assert' Expression Marker Marker ';'
AssertStatement ::= 'assert' Expression ':' Expression ';'
BreakStatement ::= 'break' Identifieropt ';'
ContinueStatement ::= 'continue' Identifieropt ';'
ReturnStatement ::= 'return' Expressionopt ';'
ThrowStatement ::= 'throw' Expression ';'
SynchronizedStatement ::= 'synchronized' '(' Expression ')' Block
TryStatement ::= 'try' Block Catches Marker
TryStatement ::= 'try' Block Catchesopt Finally
Catches ::= CatchClause
Catches ::= Catches CatchClause
Catchesopt ::= %empty
Catchesopt ::= Catches
CatchClause ::= 'catch' '(' FormalParameter ')' Block
Finally ::= 'finally' Block
Primary ::= PrimaryNoNewArray
Primary ::= ArrayCreationUninitialized
Primary ::= ArrayCreationInitialized
PrimaryNoNewArray ::= Literal
PrimaryNoNewArray ::= 'this'
PrimaryNoNewArray ::= '(' Name Marker ')'
PrimaryNoNewArray ::= '(' ExpressionNotName Marker ')'
PrimaryNoNewArray ::= ClassInstanceCreationExpression
PrimaryNoNewArray ::= FieldAccess
PrimaryNoNewArray ::= Name '.' 'this'
PrimaryNoNewArray ::= PrimitiveType Dimsopt '.' 'class'
PrimaryNoNewArray ::= Name Dims '.' 'class'
PrimaryNoNewArray ::= Name '.' Marker 'class'
PrimaryNoNewArray ::= VoidType '.' Marker 'class'
PrimaryNoNewArray ::= MethodInvocation
PrimaryNoNewArray ::= ArrayAccess
ClassInstanceCreationExpression ::= 'new' ClassOrInterfaceType Arguments ClassBodyopt
ClassInstanceCreationExpression ::= 'new' TypeArguments ClassOrInterfaceType Arguments ClassBodyopt
ClassInstanceCreationExpression ::= Primary '.' 'new' TypeArgumentsopt 'Identifier' TypeArgumentsopt Arguments ClassBodyopt
ClassInstanceCreationExpression ::= Name '.' 'new' TypeArgumentsopt 'Identifier' TypeArgumentsopt Arguments ClassBodyopt
ArgumentList ::= Expression
ArgumentList ::= ArgumentList ',' Expression
ArgumentListopt ::= %empty
ArgumentListopt ::= ArgumentList
ArrayCreationUninitialized ::= 'new' PrimitiveType DimExprs Dimsopt
ArrayCreationUninitialized ::= 'new' ClassOrInterfaceType DimExprs Dimsopt
ArrayCreationInitialized ::= 'new' PrimitiveType Dims ArrayInitializer
ArrayCreationInitialized ::= 'new' ClassOrInterfaceType Dims ArrayInitializer
DimExprs ::= DimExpr
DimExprs ::= DimExprs DimExpr
DimExpr ::= '[' Expression ']'
Dims ::= '[' ']'
Dims ::= Dims '[' ']'
Dimsopt ::= %empty
Dimsopt ::= Dims
SuperAccess ::= 'super'
SuperAccess ::= Name '.' Marker 'super'
FieldAccess ::= Primary '.' Marker 'Identifier'
FieldAccess ::= SuperAccess '.' Marker 'Identifier'
MethodInvocation ::= 'Identifier' Arguments
MethodInvocation ::= Name '.' Marker 'Identifier' Arguments
MethodInvocation ::= Name '.' TypeArguments 'Identifier' Arguments
MethodInvocation ::= Primary '.' Marker 'Identifier' Arguments
MethodInvocation ::= Primary '.' TypeArguments 'Identifier' Arguments
MethodInvocation ::= SuperAccess '.' Marker 'Identifier' Arguments
MethodInvocation ::= SuperAccess '.' TypeArguments 'Identifier' Arguments
ArrayAccess ::= Name '[' Expression ']'
ArrayAccess ::= PrimaryNoNewArray '[' Expression ']'
ArrayAccess ::= ArrayCreationInitialized '[' Expression ']'
PostfixExpression ::= Primary
PostfixExpression ::= Name
PostfixExpression ::= PostIncrementExpression
PostfixExpression ::= PostDecrementExpression
PostfixExpressionNotName ::= Primary
PostfixExpressionNotName ::= PostIncrementExpression
PostfixExpressionNotName ::= PostDecrementExpression
PostIncrementExpression ::= PostfixExpression '++'
PostDecrementExpression ::= PostfixExpression '--'
UnaryExpression ::= PreIncrementExpression
UnaryExpression ::= PreDecrementExpression
UnaryExpression ::= '+' UnaryExpression
UnaryExpression ::= '-' UnaryExpression
UnaryExpression ::= UnaryExpressionNotPlusMinus
UnaryExpressionNotName ::= PreIncrementExpression
UnaryExpressionNotName ::= PreDecrementExpression
UnaryExpressionNotName ::= '+' UnaryExpression
UnaryExpressionNotName ::= '-' UnaryExpression
UnaryExpressionNotName ::= UnaryExpressionNotPlusMinusNotName
PreIncrementExpression ::= '++' UnaryExpression
PreDecrementExpression ::= '--' UnaryExpression
UnaryExpressionNotPlusMinus ::= PostfixExpression
UnaryExpressionNotPlusMinus ::= '~' UnaryExpression
UnaryExpressionNotPlusMinus ::= '!' UnaryExpression
UnaryExpressionNotPlusMinus ::= CastExpression
UnaryExpressionNotPlusMinusNotName ::= PostfixExpressionNotName
UnaryExpressionNotPlusMinusNotName ::= '~' UnaryExpression
UnaryExpressionNotPlusMinusNotName ::= '!' UnaryExpression
UnaryExpressionNotPlusMinusNotName ::= CastExpression
CastExpression ::= '(' PrimitiveType Dimsopt ')' UnaryExpression
CastExpression ::= '(' Name Marker ')' UnaryExpressionNotPlusMinus
CastExpression ::= '(' Name Dims ')' UnaryExpressionNotPlusMinus
CastExpression ::= '(' Name '<' TypeArgumentList1 Dimsopt ')' UnaryExpressionNotPlusMinus
CastExpression ::= '(' Name '<' TypeArgumentList1 '.' ClassOrInterfaceType Dimsopt ')' UnaryExpressionNotPlusMinus
MultiplicativeExpression ::= UnaryExpression
MultiplicativeExpression ::= MultiplicativeExpression '*' UnaryExpression
MultiplicativeExpression ::= MultiplicativeExpression '/' UnaryExpression
MultiplicativeExpression ::= MultiplicativeExpression '%' UnaryExpression
MultiplicativeExpressionNotName ::= UnaryExpressionNotName
MultiplicativeExpressionNotName ::= MultiplicativeExpressionNotName '*' UnaryExpression
MultiplicativeExpressionNotName ::= Name '*' UnaryExpression
MultiplicativeExpressionNotName ::= MultiplicativeExpressionNotName '/' UnaryExpression
MultiplicativeExpressionNotName ::= Name '/' UnaryExpression
MultiplicativeExpressionNotName ::= MultiplicativeExpressionNotName '%' UnaryExpression
MultiplicativeExpressionNotName ::= Name '%' UnaryExpression
AdditiveExpression ::= MultiplicativeExpression
AdditiveExpression ::= AdditiveExpression '+' MultiplicativeExpression
AdditiveExpression ::= AdditiveExpression '-' MultiplicativeExpression
AdditiveExpressionNotName ::= MultiplicativeExpressionNotName
AdditiveExpressionNotName ::= AdditiveExpressionNotName '+' MultiplicativeExpression
AdditiveExpressionNotName ::= Name '+' MultiplicativeExpression
AdditiveExpressionNotName ::= AdditiveExpressionNotName '-' MultiplicativeExpression
AdditiveExpressionNotName ::= Name '-' MultiplicativeExpression
ShiftExpression ::= AdditiveExpression
ShiftExpression ::= ShiftExpression '<<' AdditiveExpression
ShiftExpression ::= ShiftExpression '>>' AdditiveExpression
ShiftExpression ::= ShiftExpression '>>>' AdditiveExpression
ShiftExpressionNotName ::= AdditiveExpressionNotName
ShiftExpressionNotName ::= ShiftExpressionNotName '<<' AdditiveExpression
ShiftExpressionNotName ::= Name '<<' AdditiveExpression
ShiftExpressionNotName ::= ShiftExpressionNotName '>>' AdditiveExpression
ShiftExpressionNotName ::= Name '>>' AdditiveExpression
ShiftExpressionNotName ::= ShiftExpressionNotName '>>>' AdditiveExpression
ShiftExpressionNotName ::= Name '>>>' AdditiveExpression
RelationalExpression ::= ShiftExpression
RelationalExpression ::= ShiftExpression '<' ShiftExpression
RelationalExpression ::= RelationalExpression '>' ShiftExpression
RelationalExpression ::= RelationalExpression '<=' ShiftExpression
RelationalExpression ::= RelationalExpression '>=' ShiftExpression
RelationalExpression ::= RelationalExpression 'instanceof' ReferenceType
RelationalExpressionNotName ::= ShiftExpressionNotName
RelationalExpressionNotName ::= ShiftExpressionNotName '<' ShiftExpression
RelationalExpressionNotName ::= Name '<' ShiftExpression
RelationalExpressionNotName ::= ShiftExpressionNotName '>' ShiftExpression
RelationalExpressionNotName ::= Name '>' ShiftExpression
RelationalExpressionNotName ::= RelationalExpressionNotName '<=' ShiftExpression
RelationalExpressionNotName ::= Name '<=' ShiftExpression
RelationalExpressionNotName ::= RelationalExpressionNotName '>=' ShiftExpression
RelationalExpressionNotName ::= Name '>=' ShiftExpression
RelationalExpressionNotName ::= RelationalExpressionNotName 'instanceof' ReferenceType
RelationalExpressionNotName ::= Name 'instanceof' ReferenceType
EqualityExpression ::= RelationalExpression
EqualityExpression ::= EqualityExpression '==' RelationalExpression
EqualityExpression ::= EqualityExpression '!=' RelationalExpression
EqualityExpressionNotName ::= RelationalExpressionNotName
EqualityExpressionNotName ::= EqualityExpressionNotName '==' RelationalExpression
EqualityExpressionNotName ::= Name '==' RelationalExpression
EqualityExpressionNotName ::= EqualityExpressionNotName '!=' RelationalExpression
EqualityExpressionNotName ::= Name '!=' RelationalExpression
AndExpression ::= EqualityExpression
AndExpression ::= AndExpression '&' EqualityExpression
AndExpressionNotName ::= EqualityExpressionNotName
AndExpressionNotName ::= AndExpressionNotName '&' EqualityExpression
AndExpressionNotName ::= Name '&' EqualityExpression
ExclusiveOrExpression ::= AndExpression
ExclusiveOrExpression ::= ExclusiveOrExpression '^' AndExpression
ExclusiveOrExpressionNotName ::= AndExpressionNotName
ExclusiveOrExpressionNotName ::= ExclusiveOrExpressionNotName '^' AndExpression
ExclusiveOrExpressionNotName ::= Name '^' AndExpression
InclusiveOrExpression ::= ExclusiveOrExpression
InclusiveOrExpression ::= InclusiveOrExpression '|' ExclusiveOrExpression
InclusiveOrExpressionNotName ::= ExclusiveOrExpressionNotName
InclusiveOrExpressionNotName ::= InclusiveOrExpressionNotName '|' ExclusiveOrExpression
InclusiveOrExpressionNotName ::= Name '|' ExclusiveOrExpression
ConditionalAndExpression ::= InclusiveOrExpression
ConditionalAndExpression ::= ConditionalAndExpression '&&' InclusiveOrExpression
ConditionalAndExpressionNotName ::= InclusiveOrExpressionNotName
ConditionalAndExpressionNotName ::= ConditionalAndExpressionNotName '&&' InclusiveOrExpression
ConditionalAndExpressionNotName ::= Name '&&' InclusiveOrExpression
ConditionalOrExpression ::= ConditionalAndExpression
ConditionalOrExpression ::= ConditionalOrExpression '||' ConditionalAndExpression
ConditionalOrExpressionNotName ::= ConditionalAndExpressionNotName
ConditionalOrExpressionNotName ::= ConditionalOrExpressionNotName '||' ConditionalAndExpression
ConditionalOrExpressionNotName ::= Name '||' ConditionalAndExpression
ConditionalExpression ::= ConditionalOrExpression
ConditionalExpression ::= ConditionalOrExpression '?' Expression ':' ConditionalExpression
ConditionalExpressionNotName ::= ConditionalOrExpressionNotName
ConditionalExpressionNotName ::= ConditionalOrExpressionNotName '?' Expression ':' ConditionalExpression
ConditionalExpressionNotName ::= Name '?' Expression ':' ConditionalExpression
AssignmentExpression ::= ConditionalExpression
AssignmentExpression ::= Assignment
AssignmentExpressionNotName ::= ConditionalExpressionNotName
AssignmentExpressionNotName ::= Assignment
Assignment ::= PostfixExpression AssignmentOperator AssignmentExpression
AssignmentOperator ::= '='
AssignmentOperator ::= '*='
AssignmentOperator ::= '/='
AssignmentOperator ::= '%='
AssignmentOperator ::= '+='
AssignmentOperator ::= '-='
AssignmentOperator ::= '<<='
AssignmentOperator ::= '>>='
AssignmentOperator ::= '>>>='
AssignmentOperator ::= '&='
AssignmentOperator ::= '^='
AssignmentOperator ::= '|='
Expression ::= AssignmentExpression
Expressionopt ::= %empty
Expressionopt ::= Expression
ExpressionNotName ::= AssignmentExpressionNotName
Marker ::= %empty
,opt ::= %empty
,opt ::= ','
Identifieropt ::= %empty
Identifieropt ::= 'Identifier'
PackageHeaderMarker ::= %empty
MethodHeaderMarker ::= %empty
TypeArguments ::= '<' TypeArgumentList1
TypeArgumentsopt ::= %empty
TypeArgumentsopt ::= TypeArguments
Wildcard ::= '?' Marker Marker Marker
Wildcard ::= '?' 'extends' Marker ReferenceType
Wildcard ::= '?' Marker 'super' ReferenceType
Wildcard1 ::= '?' Marker Marker '>'
Wildcard1 ::= '?' 'extends' Marker ReferenceType1
Wildcard1 ::= '?' Marker 'super' ReferenceType1
Wildcard2 ::= '?' Marker Marker '>>'
Wildcard2 ::= '?' 'extends' Marker ReferenceType2
Wildcard2 ::= '?' Marker 'super' ReferenceType2
Wildcard3 ::= '?' Marker Marker '>>>'
Wildcard3 ::= '?' 'extends' Marker ReferenceType3
Wildcard3 ::= '?' Marker 'super' ReferenceType3
TypeArgumentList ::= TypeArgument
TypeArgumentList ::= TypeArgumentList ',' TypeArgument
TypeArgumentList1 ::= TypeArgument1
TypeArgumentList1 ::= TypeArgumentList ',' TypeArgument1
TypeArgumentList2 ::= TypeArgument2
TypeArgumentList2 ::= TypeArgumentList ',' TypeArgument2
TypeArgumentList3 ::= TypeArgument3
TypeArgumentList3 ::= TypeArgumentList ',' TypeArgument3
TypeArgument ::= ReferenceType
TypeArgument ::= Wildcard
TypeArgument1 ::= ReferenceType1
TypeArgument1 ::= Wildcard1
TypeArgument2 ::= ReferenceType2
TypeArgument2 ::= Wildcard2
TypeArgument3 ::= ReferenceType3
TypeArgument3 ::= Wildcard3
ReferenceType1 ::= ReferenceType '>'
ReferenceType1 ::= ClassOrInterface '<' TypeArgumentList2 Marker
ReferenceType2 ::= ReferenceType '>>'
ReferenceType2 ::= ClassOrInterface '<' TypeArgumentList3 Marker
ReferenceType3 ::= ReferenceType '>>>'
TypeParameters ::= '<' TypeParameterList1
TypeParametersopt ::= %empty
TypeParametersopt ::= TypeParameters
TypeParameterList ::= TypeParameter
TypeParameterList ::= TypeParameterList ',' TypeParameter
TypeParameterList1 ::= TypeParameter1
TypeParameterList1 ::= TypeParameterList ',' TypeParameter1
TypeParameter ::= 'Identifier' TypeBoundopt
TypeParameter1 ::= 'Identifier' Marker '>'
TypeParameter1 ::= 'Identifier' TypeBound1
TypeBound ::= 'extends' ReferenceType AdditionalBoundListopt
TypeBoundopt ::= %empty
TypeBoundopt ::= TypeBound
TypeBound1 ::= 'extends' ReferenceType1 Marker
TypeBound1 ::= 'extends' ReferenceType AdditionalBoundList1
AdditionalBoundList ::= AdditionalBound
AdditionalBoundList ::= AdditionalBoundList AdditionalBound
AdditionalBoundListopt ::= %empty
AdditionalBoundListopt ::= AdditionalBoundList
AdditionalBoundList1 ::= AdditionalBound1
AdditionalBoundList1 ::= AdditionalBoundList AdditionalBound1
AdditionalBound ::= '&' ClassOrInterfaceType
AdditionalBound1 ::= '&' ClassOrInterfaceType1
ClassOrInterfaceType1 ::= ClassOrInterfaceType '>'
ClassOrInterfaceType1 ::= ClassOrInterface '<' TypeArgumentList2 Marker

-- =========================================================================
-- Java 7 Features (Rules 567+)
-- =========================================================================

-- Java 7: Try-with-resources
-- Rule 567: TryStatement with resources and optional catches (no finally)
TryStatement ::= 'try' ResourceSpecification Block Catchesopt Marker

-- Rule 568: TryStatement with resources and optional catches and finally
TryStatement ::= 'try' ResourceSpecification Block Catchesopt Finally

-- Rule 569: ResourceSpecification without trailing semicolon
ResourceSpecification ::= '(' Resources Marker ')'

-- Rule 570: ResourceSpecification with trailing semicolon
ResourceSpecification ::= '(' Resources ';' ')'

-- Rule 571: Single resource
Resources ::= Resource

-- Rule 572: Multiple resources
Resources ::= Resources ';' Resource

-- Rule 573: Resource without modifiers
Resource ::= Type VariableDeclaratorId '=' Expression

-- Rule 574: Resource with modifiers (e.g., final)
Resource ::= Modifiers Type VariableDeclaratorId '=' Expression

-- Java 7: Multi-catch - extends FormalParameter to support union types
-- The original CatchClause (rule 308) uses FormalParameter, which uses Type.
-- For multi-catch, we add a new production that allows union types.

-- Rule 575: CatchClause with union catch type (multi-catch)
CatchClause ::= 'catch' '(' UnionType VariableDeclaratorId ')' Block

-- Rule 576: CatchClause with union catch type and modifiers
CatchClause ::= 'catch' '(' Modifiers UnionType VariableDeclaratorId ')' Block

-- Rule 577: Union type - at least two exception types joined with |
UnionType ::= ClassOrInterfaceType '|' ClassOrInterfaceType

-- Rule 578: Union type - additional types
UnionType ::= UnionType '|' ClassOrInterfaceType

-- Java 7: Diamond operator
-- Rule 580: new Type<>(...) without explicit type arguments
ClassInstanceCreationExpression ::= 'new' ClassOrInterfaceType DiamondMarker Arguments ClassBodyopt

-- Rule 581: new <T>Type<>(...) with constructor type args but diamond for class
ClassInstanceCreationExpression ::= 'new' TypeArguments ClassOrInterfaceType DiamondMarker Arguments ClassBodyopt

-- Rule 582: Qualified new with diamond
ClassInstanceCreationExpression ::= Primary '.' 'new' TypeArgumentsopt 'Identifier' DiamondMarker Arguments ClassBodyopt

-- Rule 583: Qualified new with diamond (via name)
ClassInstanceCreationExpression ::= Name '.' 'new' TypeArgumentsopt 'Identifier' DiamondMarker Arguments ClassBodyopt

-- Rule 584: DiamondMarker matches '<>'
DiamondMarker ::= '<' '>'

%End
