-- Java Grammar for jikespg
-- Generated from java.g

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
Literal ::= StringLiteral
Literal ::= 'null'
BooleanLiteral ::= 'true'
BooleanLiteral ::= 'false'
Type ::= PrimitiveType
NumericType ::= IntegralType
IntegralType ::= 'short'
IntegralType ::= 'int'
IntegralType ::= 'long'
IntegralType ::= 'char'
FloatingPointType ::= 'float'
FloatingPointType ::= 'double'
VoidType ::= 'void'
ReferenceType ::= ClassOrInterfaceType
ClassOrInterface ::= Name
ClassOrInterface ::= ClassOrInterface '<' TypeArgumentList1 '.' Name
ArrayType ::= PrimitiveType Dims
ArrayType ::= Name Dims
ArrayType ::= ClassOrInterface '<' TypeArgumentList1 '.' Name Dims
ArrayType ::= ClassOrInterface '<' TypeArgumentList1 Dims
Name ::= 'Identifier'
Name ::= Name '.' Marker 'Identifier'
CompilationUnit ::= PackageDeclaration ImportDeclarationsopt
CompilationUnit ::= Marker ImportDeclarations TypeDeclarationsopt
CompilationUnit ::= TypeDeclarationsopt
ImportDeclarations ::= ImportDeclaration
ImportDeclarations ::= ImportDeclarations ImportDeclaration
ImportDeclarationsopt ::= $empty
ImportDeclarationsopt ::= ImportDeclarations
PackageDeclaration ::= Modifiers 'package' Name PackageHeaderMarker ';'
ImportDeclaration ::= SingleTypeImportDeclaration
TypeImportOnDemandDeclaration ::= 'import' Marker Name '.' '*' ';'
SingleStaticImportDeclaration ::= 'import' 'static' Name Marker Marker ';'
StaticImportOnDemandDeclaration ::= 'import' 'static' Name '.' '*' ';'
TypeDeclaration ::= ClassDeclaration
Modifiers ::= Modifier
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
MemberValuePairs ::= MemberValuePair
MemberValue ::= ConditionalExpression
MemberValueArrayInitializer ::= '{' MemberValues ,opt '}'
MemberValues ::= MemberValue
SingleMemberAnnotation ::= '@' Name '(' MemberValue ')'
ClassDeclaration ::= Marker 'class' 'Identifier' TypeParametersopt
ClassDeclaration ::= Modifiers 'class' 'Identifier' TypeParametersopt
Super ::= 'extends' ClassOrInterfaceType
Superopt ::= $empty
Interfacesopt ::= $empty
ClassBodyopt ::= $empty
FieldDeclaration ::= Modifiers Marker Type VariableDeclarators ';'
VariableDeclarators ::= VariableDeclarator
VariableDeclarator ::= VariableDeclaratorId '=' VariableInitializer
VariableDeclaratorId ::= 'Identifier' Dimsopt
VariableInitializer ::= Expression
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
FormalParameter ::= Modifiers Type Marker VariableDeclaratorId
LastFormalParameter ::= FormalParameter
LastFormalParameter ::= Modifiers Type '...' VariableDeclaratorId
Throws ::= 'throws' TypeList
Throwsopt ::= $empty
InitializerDeclaration ::= Marker MethodHeaderMarker MethodBody
InitializerDeclaration ::= Modifiers MethodHeaderMarker MethodBody
ConstructorDeclaration ::= Marker Marker ConstructorDeclarator Throwsopt
ConstructorDeclaration ::= Modifiers Marker ConstructorDeclarator Throwsopt
ConstructorDeclaration ::= Marker TypeParameters ConstructorDeclarator
ConstructorDeclaration ::= Modifiers TypeParameters ConstructorDeclarator
ConstructorDeclarator ::= 'Identifier' '(' FormalParameterListopt ')' Marker
ExplicitConstructorInvocation ::= 'this' Arguments ';'
ExplicitConstructorInvocation ::= TypeArguments 'this' Arguments ';'
ExplicitConstructorInvocation ::= 'super' Arguments ';'
ExplicitConstructorInvocation ::= TypeArguments 'super' Arguments ';'
ExplicitConstructorInvocation ::= Primary '.' TypeArgumentsopt 'super'
ExplicitConstructorInvocation ::= Name '.' Marker 'super' Arguments ';'
ExplicitConstructorInvocation ::= Name '.' TypeArguments 'super' Arguments ';'
EnumDeclaration ::= Marker 'enum' 'Identifier' Interfacesopt EnumBody
EnumDeclaration ::= Modifiers 'enum' 'Identifier' Interfacesopt EnumBody
EnumBody ::= '{' Marker ,opt EnumBodyDeclarationsopt '}'
EnumBody ::= '{' EnumConstants ,opt EnumBodyDeclarationsopt '}'
EnumConstants ::= EnumConstant
Arguments ::= '(' ArgumentListopt ')'
Argumentsopt ::= $empty
EnumBodyDeclarationsopt ::= $empty
InterfaceDeclaration ::= Modifiers 'interface' 'Identifier' TypeParametersopt
ExtendsInterfaces ::= 'extends' TypeList
ExtendsInterfacesopt ::= $empty
InterfaceMemberDeclarations ::= InterfaceMemberDeclaration
InterfaceMemberDeclaration ::= MethodDeclaration
AnnotationTypeDeclaration ::= Modifiers '@' 'interface' 'Identifier'
AnnotationTypeBody ::= '{' AnnotationTypeMemberDeclarationsopt '}'
AnnotationTypeMemberDeclarations ::= AnnotationTypeMemberDeclaration
AnnotationTypeMemberDeclaration ::= Modifiers Marker Type 'Identifier' '(' ')'
AnnotationTypeMemberDeclaration ::= ConstantDeclaration
DefaultValueopt ::= $empty
ArrayInitializer ::= '{' VariableInitializers ,opt '}'
VariableInitializers ::= VariableInitializer
BlockStatements ::= BlockStatement
BlockStatement ::= EnumDeclaration
BlockStatement ::= ExplicitConstructorInvocation
LocalVariableDeclaration ::= Type Marker Marker VariableDeclarators
LocalVariableDeclaration ::= Modifiers Type Marker VariableDeclarators
Statement ::= StatementWithoutTrailingSubstatement
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
IfThenElseStatement ::= 'if' '(' Expression ')' StatementNoShortIf
IfThenElseStatementNoShortIf ::= 'if' '(' Expression ')' StatementNoShortIf
SwitchStatement ::= 'switch' '(' Expression ')' SwitchBlock
SwitchBlock ::= '{' SwitchBlockStatements SwitchLabelsopt '}'
SwitchBlock ::= '{' SwitchLabelsopt '}'
SwitchBlockStatements ::= SwitchBlockStatement
SwitchLabels ::= SwitchLabel
SwitchLabel ::= 'default' Marker ':'
WhileStatement ::= 'while' '(' Expression ')' Statement
WhileStatementNoShortIf ::= 'while' '(' Expression ')' StatementNoShortIf
DoStatement ::= 'do' Statement 'while' '(' Expression ')' ';'
ForStatement ::= 'for' '(' ForInitopt ';' Expressionopt ';' ForUpdateopt ')'
ForStatementNoShortIf ::= 'for' '(' ForInitopt ';' Expressionopt ';'
ForInit ::= StatementExpressionList
ForeachStatementNoShortIf ::= 'for' '(' FormalParameter ':' Expression ')'
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
Finally ::= 'finally' Block
Primary ::= PrimaryNoNewArray
PrimaryNoNewArray ::= '(' Name Marker ')'
PrimaryNoNewArray ::= '(' ExpressionNotName Marker ')'
PrimaryNoNewArray ::= ClassInstanceCreationExpression
PrimaryNoNewArray ::= PrimitiveType Dimsopt '.' 'class'
PrimaryNoNewArray ::= Name Dims '.' 'class'
PrimaryNoNewArray ::= Name '.' Marker 'class'
PrimaryNoNewArray ::= VoidType '.' Marker 'class'
PrimaryNoNewArray ::= MethodInvocation
ClassInstanceCreationExpression ::= 'new' TypeArguments ClassOrInterfaceType
ClassInstanceCreationExpression ::= Primary '.' 'new' TypeArgumentsopt
ClassInstanceCreationExpression ::= Name '.' 'new' TypeArgumentsopt
ArgumentList ::= Expression
ArrayCreationUninitialized ::= 'new' ClassOrInterfaceType DimExprs Dimsopt
ArrayCreationInitialized ::= 'new' PrimitiveType Dims ArrayInitializer
ArrayCreationInitialized ::= 'new' ClassOrInterfaceType Dims ArrayInitializer
DimExprs ::= DimExpr
Dims ::= '[' ']'
Dims ::= Dims '[' ']'
Dimsopt ::= $empty
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
PostDecrementExpression ::= PostfixExpression '--'
UnaryExpression ::= PreIncrementExpression
UnaryExpression ::= '-' UnaryExpression
CastExpression ::= '(' Name Marker ')' UnaryExpressionNotPlusMinus
CastExpression ::= '(' Name Dims ')' UnaryExpressionNotPlusMinus
CastExpression ::= '(' Name '<' TypeArgumentList1 Dimsopt ')'
CastExpression ::= '(' Name '<' TypeArgumentList1 '.' ClassOrInterfaceType
MultiplicativeExpression ::= UnaryExpression
MultiplicativeExpression ::= MultiplicativeExpression '/' UnaryExpression
RelationalExpressionNotName ::= ShiftExpressionNotName
RelationalExpressionNotName ::= Name 'instanceof' ReferenceType
EqualityExpression ::= RelationalExpression
ConditionalExpressionNotName ::= ConditionalOrExpressionNotName
ConditionalExpressionNotName ::= Name '?' Expression ':' ConditionalExpression
AssignmentExpression ::= ConditionalExpression
AssignmentOperator ::= '='
MethodHeaderMarker ::= $empty
TypeArguments ::= '<' TypeArgumentList1
TypeArgumentsopt ::= $empty
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
ReferenceType2 ::= ReferenceType '>>'
ReferenceType3 ::= ReferenceType '>>>'
TypeParametersopt ::= $empty
TypeParameter1 ::= 'Identifier' Marker '>'
TypeParameter1 ::= 'Identifier' TypeBound1
TypeBound ::= 'extends' ReferenceType AdditionalBoundListopt
TypeBoundopt ::= $empty
TypeBound1 ::= 'extends' ReferenceType AdditionalBoundList1
AdditionalBoundList ::= AdditionalBound
AdditionalBound1 ::= '&' ClassOrInterfaceType1
ClassOrInterfaceType1 ::= ClassOrInterfaceType '>'

%End
