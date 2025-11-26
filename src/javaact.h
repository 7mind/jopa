#ifndef HEADERS


namespace Jopa { // Open namespace Jopa block
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

#ifndef HEADERS
    rule_action[1] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[2] = &Parser::SetSym1ToSym2;
#endif

#ifndef HEADERS
    rule_action[3] = &Parser::Act3;
#else
    void Act3();
#endif

#ifndef HEADERS
    rule_action[4] = &Parser::Act4;
#else
    void Act4();
#endif

#ifndef HEADERS
    rule_action[5] = &Parser::Act5;
#else
    void Act5();
#endif

#ifndef HEADERS
    rule_action[6] = &Parser::Act6;
#else
    void Act6();
#endif

#ifndef HEADERS
    rule_action[7] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[8] = &Parser::Act8;
#else
    void Act8();
#endif

#ifndef HEADERS
    rule_action[9] = &Parser::Act9;
#else
    void Act9();
#endif

#ifndef HEADERS
    rule_action[10] = &Parser::Act10;
#else
    void Act10();
#endif

#ifndef HEADERS
    rule_action[11] = &Parser::Act11;
#else
    void Act11();
#endif

#ifndef HEADERS
    rule_action[12] = &Parser::Act12;
#else
    void Act12();
#endif

#ifndef HEADERS
    rule_action[13] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[14] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[15] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[16] = &Parser::Act16;
#else
    void Act16();
#endif

#ifndef HEADERS
    rule_action[17] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[18] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[19] = &Parser::Act19;
#else
    void Act19();
#endif

#ifndef HEADERS
    rule_action[20] = &Parser::Act20;
#else
    void Act20();
#endif

#ifndef HEADERS
    rule_action[21] = &Parser::Act21;
#else
    void Act21();
#endif

#ifndef HEADERS
    rule_action[22] = &Parser::Act22;
#else
    void Act22();
#endif

#ifndef HEADERS
    rule_action[23] = &Parser::Act23;
#else
    void Act23();
#endif

#ifndef HEADERS
    rule_action[24] = &Parser::Act24;
#else
    void Act24();
#endif

#ifndef HEADERS
    rule_action[25] = &Parser::Act25;
#else
    void Act25();
#endif

#ifndef HEADERS
    rule_action[26] = &Parser::Act26;
#else
    void Act26();
#endif

#ifndef HEADERS
    rule_action[27] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[28] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[29] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[30] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[31] = &Parser::Act31;
#else
    void Act31();
#endif

#ifndef HEADERS
    rule_action[32] = &Parser::Act32;
#else
    void Act32();
#endif

#ifndef HEADERS
    rule_action[33] = &Parser::MakeArrayType;
#endif

#ifndef HEADERS
    rule_action[34] = &Parser::MakeArrayType;
#endif

#ifndef HEADERS
    rule_action[35] = &Parser::Act35;
#else
    void Act35();
#endif

#ifndef HEADERS
    rule_action[36] = &Parser::Act36;
#else
    void Act36();
#endif

#ifndef HEADERS
    rule_action[37] = &Parser::Act37;
#else
    void Act37();
#endif

#ifndef HEADERS
    rule_action[38] = &Parser::Act38;
#else
    void Act38();
#endif

#ifndef HEADERS
    rule_action[39] = &Parser::MakeCompilationUnit;
#endif

#ifndef HEADERS
    rule_action[40] = &Parser::MakeCompilationUnit;
#endif

#ifndef HEADERS
    rule_action[41] = &Parser::Act41;
#else
    void Act41();
#endif

#ifndef HEADERS
    rule_action[42] = &Parser::StartList;
#endif

#ifndef HEADERS
    rule_action[43] = &Parser::AddList2;
#endif

#ifndef HEADERS
    rule_action[44] = &Parser::NullAction;
#endif

#ifndef HEADERS
    rule_action[45] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[46] = &Parser::StartList;
#endif

#ifndef HEADERS
    rule_action[47] = &Parser::AddList2;
#endif

#ifndef HEADERS
    rule_action[48] = &Parser::NullAction;
#endif

#ifndef HEADERS
    rule_action[49] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[50] = &Parser::MakePackageDeclaration;
#endif

#ifndef HEADERS
    rule_action[51] = &Parser::MakePackageDeclaration;
#endif

#ifndef HEADERS
    rule_action[52] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[53] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[54] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[55] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[56] = &Parser::MakeImportDeclaration;
#endif

#ifndef HEADERS
    rule_action[57] = &Parser::MakeImportDeclaration;
#endif

#ifndef HEADERS
    rule_action[58] = &Parser::MakeImportDeclaration;
#endif

#ifndef HEADERS
    rule_action[59] = &Parser::MakeImportDeclaration;
#endif

#ifndef HEADERS
    rule_action[60] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[61] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[62] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[63] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[64] = &Parser::Act64;
#else
    void Act64();
#endif

#ifndef HEADERS
    rule_action[65] = &Parser::StartList;
#endif

#ifndef HEADERS
    rule_action[66] = &Parser::AddList2;
#endif

#ifndef HEADERS
    rule_action[67] = &Parser::NullAction;
#endif

#ifndef HEADERS
    rule_action[68] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[69] = &Parser::MakeModifier;
#endif

#ifndef HEADERS
    rule_action[70] = &Parser::MakeModifier;
#endif

#ifndef HEADERS
    rule_action[71] = &Parser::MakeModifier;
#endif

#ifndef HEADERS
    rule_action[72] = &Parser::MakeModifier;
#endif

#ifndef HEADERS
    rule_action[73] = &Parser::MakeModifier;
#endif

#ifndef HEADERS
    rule_action[74] = &Parser::MakeModifier;
#endif

#ifndef HEADERS
    rule_action[75] = &Parser::MakeModifier;
#endif

#ifndef HEADERS
    rule_action[76] = &Parser::MakeModifier;
#endif

#ifndef HEADERS
    rule_action[77] = &Parser::MakeModifier;
#endif

#ifndef HEADERS
    rule_action[78] = &Parser::MakeModifier;
#endif

#ifndef HEADERS
    rule_action[79] = &Parser::MakeModifier;
#endif

#ifndef HEADERS
    rule_action[80] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[81] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[82] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[83] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[84] = &Parser::MakeAnnotation;
#endif

#ifndef HEADERS
    rule_action[85] = &Parser::StartList;
#endif

#ifndef HEADERS
    rule_action[86] = &Parser::AddList3;
#endif

#ifndef HEADERS
    rule_action[87] = &Parser::NullAction;
#endif

#ifndef HEADERS
    rule_action[88] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[89] = &Parser::Act89;
#else
    void Act89();
#endif

#ifndef HEADERS
    rule_action[90] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[91] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[92] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[93] = &Parser::MakeArrayInitializer;
#endif

#ifndef HEADERS
    rule_action[94] = &Parser::MakeArrayInitializer;
#endif

#ifndef HEADERS
    rule_action[95] = &Parser::StartList;
#endif

#ifndef HEADERS
    rule_action[96] = &Parser::AddList3;
#endif

#ifndef HEADERS
    rule_action[97] = &Parser::MakeAnnotation;
#endif

#ifndef HEADERS
    rule_action[98] = &Parser::Act98;
#else
    void Act98();
#endif

#ifndef HEADERS
    rule_action[99] = &Parser::MakeClassDeclaration;
#endif

#ifndef HEADERS
    rule_action[100] = &Parser::MakeClassDeclaration;
#endif

#ifndef HEADERS
    rule_action[101] = &Parser::SetSym1ToSym2;
#endif

#ifndef HEADERS
    rule_action[102] = &Parser::NullAction;
#endif

#ifndef HEADERS
    rule_action[103] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[104] = &Parser::SetSym1ToSym2;
#endif

#ifndef HEADERS
    rule_action[105] = &Parser::NullAction;
#endif

#ifndef HEADERS
    rule_action[106] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[107] = &Parser::StartList;
#endif

#ifndef HEADERS
    rule_action[108] = &Parser::AddList3;
#endif

#ifndef HEADERS
    rule_action[109] = &Parser::MakeClassBody;
#endif

#ifndef HEADERS
    rule_action[110] = &Parser::NullAction;
#endif

#ifndef HEADERS
    rule_action[111] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[112] = &Parser::StartList;
#endif

#ifndef HEADERS
    rule_action[113] = &Parser::AddList2;
#endif

#ifndef HEADERS
    rule_action[114] = &Parser::NullAction;
#endif

#ifndef HEADERS
    rule_action[115] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[116] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[117] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[118] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[119] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[120] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[121] = &Parser::MakeFieldDeclaration;
#endif

#ifndef HEADERS
    rule_action[122] = &Parser::MakeFieldDeclaration;
#endif

#ifndef HEADERS
    rule_action[123] = &Parser::StartList;
#endif

#ifndef HEADERS
    rule_action[124] = &Parser::AddList3;
#endif

#ifndef HEADERS
    rule_action[125] = &Parser::Act125;
#else
    void Act125();
#endif

#ifndef HEADERS
    rule_action[126] = &Parser::Act126;
#else
    void Act126();
#endif

#ifndef HEADERS
    rule_action[127] = &Parser::Act127;
#else
    void Act127();
#endif

#ifndef HEADERS
    rule_action[128] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[129] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[130] = &Parser::MakeMethodDeclaration;
#endif

#ifndef HEADERS
    rule_action[131] = &Parser::MakeMethodDeclaration;
#endif

#ifndef HEADERS
    rule_action[132] = &Parser::MakeMethodHeader;
#endif

#ifndef HEADERS
    rule_action[133] = &Parser::MakeMethodHeader;
#endif

#ifndef HEADERS
    rule_action[134] = &Parser::MakeMethodHeader;
#endif

#ifndef HEADERS
    rule_action[135] = &Parser::MakeMethodHeader;
#endif

#ifndef HEADERS
    rule_action[136] = &Parser::MakeMethodHeader;
#endif

#ifndef HEADERS
    rule_action[137] = &Parser::MakeMethodHeader;
#endif

#ifndef HEADERS
    rule_action[138] = &Parser::MakeMethodHeader;
#endif

#ifndef HEADERS
    rule_action[139] = &Parser::MakeMethodHeader;
#endif

#ifndef HEADERS
    rule_action[140] = &Parser::MakeMethodDeclarator;
#endif

#ifndef HEADERS
    rule_action[141] = &Parser::StartList;
#endif

#ifndef HEADERS
    rule_action[142] = &Parser::AddList3;
#endif

#ifndef HEADERS
    rule_action[143] = &Parser::NullAction;
#endif

#ifndef HEADERS
    rule_action[144] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[145] = &Parser::StartList;
#endif

#ifndef HEADERS
    rule_action[146] = &Parser::AddList3;
#endif

#ifndef HEADERS
    rule_action[147] = &Parser::MakeFormalParameter;
#endif

#ifndef HEADERS
    rule_action[148] = &Parser::MakeFormalParameter;
#endif

#ifndef HEADERS
    rule_action[149] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[150] = &Parser::MakeFormalParameter;
#endif

#ifndef HEADERS
    rule_action[151] = &Parser::MakeFormalParameter;
#endif

#ifndef HEADERS
    rule_action[152] = &Parser::SetSym1ToSym2;
#endif

#ifndef HEADERS
    rule_action[153] = &Parser::NullAction;
#endif

#ifndef HEADERS
    rule_action[154] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[155] = &Parser::Act155;
#else
    void Act155();
#endif

#ifndef HEADERS
    rule_action[156] = &Parser::MakeInitializerDeclaration;
#endif

#ifndef HEADERS
    rule_action[157] = &Parser::MakeInitializerDeclaration;
#endif

#ifndef HEADERS
    rule_action[158] = &Parser::MakeConstructorDeclaration;
#endif

#ifndef HEADERS
    rule_action[159] = &Parser::MakeConstructorDeclaration;
#endif

#ifndef HEADERS
    rule_action[160] = &Parser::MakeConstructorDeclaration;
#endif

#ifndef HEADERS
    rule_action[161] = &Parser::MakeConstructorDeclaration;
#endif

#ifndef HEADERS
    rule_action[162] = &Parser::MakeMethodDeclarator;
#endif

#ifndef HEADERS
    rule_action[163] = &Parser::Act163;
#else
    void Act163();
#endif

#ifndef HEADERS
    rule_action[164] = &Parser::Act164;
#else
    void Act164();
#endif

#ifndef HEADERS
    rule_action[165] = &Parser::Act165;
#else
    void Act165();
#endif

#ifndef HEADERS
    rule_action[166] = &Parser::Act166;
#else
    void Act166();
#endif

#ifndef HEADERS
    rule_action[167] = &Parser::MakeQualifiedSuper;
#endif

#ifndef HEADERS
    rule_action[168] = &Parser::MakeQualifiedSuper;
#endif

#ifndef HEADERS
    rule_action[169] = &Parser::MakeQualifiedSuper;
#endif

#ifndef HEADERS
    rule_action[170] = &Parser::MakeEnumDeclaration;
#endif

#ifndef HEADERS
    rule_action[171] = &Parser::MakeEnumDeclaration;
#endif

#ifndef HEADERS
    rule_action[172] = &Parser::MakeEnumBody;
#endif

#ifndef HEADERS
    rule_action[173] = &Parser::MakeEnumBody;
#endif

#ifndef HEADERS
    rule_action[174] = &Parser::StartList;
#endif

#ifndef HEADERS
    rule_action[175] = &Parser::AddList3;
#endif

#ifndef HEADERS
    rule_action[176] = &Parser::Act176;
#else
    void Act176();
#endif

#ifndef HEADERS
    rule_action[177] = &Parser::Act177;
#else
    void Act177();
#endif

#ifndef HEADERS
    rule_action[178] = &Parser::NullAction;
#endif

#ifndef HEADERS
    rule_action[179] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[180] = &Parser::MakeClassBody;
#endif

#ifndef HEADERS
    rule_action[181] = &Parser::NullAction;
#endif

#ifndef HEADERS
    rule_action[182] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[183] = &Parser::MakeInterfaceDeclaration;
#endif

#ifndef HEADERS
    rule_action[184] = &Parser::MakeInterfaceDeclaration;
#endif

#ifndef HEADERS
    rule_action[185] = &Parser::SetSym1ToSym2;
#endif

#ifndef HEADERS
    rule_action[186] = &Parser::NullAction;
#endif

#ifndef HEADERS
    rule_action[187] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[188] = &Parser::MakeClassBody;
#endif

#ifndef HEADERS
    rule_action[189] = &Parser::StartList;
#endif

#ifndef HEADERS
    rule_action[190] = &Parser::AddList2;
#endif

#ifndef HEADERS
    rule_action[191] = &Parser::NullAction;
#endif

#ifndef HEADERS
    rule_action[192] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[193] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[194] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[195] = &Parser::Act195;
#else
    void Act195();
#endif

#ifndef HEADERS
    rule_action[196] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[197] = &Parser::MakeAnnotationTypeDeclaration;
#endif

#ifndef HEADERS
    rule_action[198] = &Parser::MakeAnnotationTypeDeclaration;
#endif

#ifndef HEADERS
    rule_action[199] = &Parser::MakeClassBody;
#endif

#ifndef HEADERS
    rule_action[200] = &Parser::StartList;
#endif

#ifndef HEADERS
    rule_action[201] = &Parser::AddList2;
#endif

#ifndef HEADERS
    rule_action[202] = &Parser::NullAction;
#endif

#ifndef HEADERS
    rule_action[203] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[204] = &Parser::MakeAnnotationTypeMemberDeclaration;
#endif

#ifndef HEADERS
    rule_action[205] = &Parser::MakeAnnotationTypeMemberDeclaration;
#endif

#ifndef HEADERS
    rule_action[206] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[207] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[208] = &Parser::SetSym1ToSym2;
#endif

#ifndef HEADERS
    rule_action[209] = &Parser::NullAction;
#endif

#ifndef HEADERS
    rule_action[210] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[211] = &Parser::MakeArrayInitializer;
#endif

#ifndef HEADERS
    rule_action[212] = &Parser::MakeArrayInitializer;
#endif

#ifndef HEADERS
    rule_action[213] = &Parser::StartList;
#endif

#ifndef HEADERS
    rule_action[214] = &Parser::AddList3;
#endif

#ifndef HEADERS
    rule_action[215] = &Parser::Act215;
#else
    void Act215();
#endif

#ifndef HEADERS
    rule_action[216] = &Parser::StartList;
#endif

#ifndef HEADERS
    rule_action[217] = &Parser::AddList2;
#endif

#ifndef HEADERS
    rule_action[218] = &Parser::NullAction;
#endif

#ifndef HEADERS
    rule_action[219] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[220] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[221] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[222] = &Parser::Act222;
#else
    void Act222();
#endif

#ifndef HEADERS
    rule_action[223] = &Parser::Act223;
#else
    void Act223();
#endif

#ifndef HEADERS
    rule_action[224] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[225] = &Parser::Act225;
#else
    void Act225();
#endif

#ifndef HEADERS
    rule_action[226] = &Parser::MakeLocalVariable;
#endif

#ifndef HEADERS
    rule_action[227] = &Parser::MakeLocalVariable;
#endif

#ifndef HEADERS
    rule_action[228] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[229] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[230] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[231] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[232] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[233] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[234] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[235] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[236] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[237] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[238] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[239] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[240] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[241] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[242] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[243] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[244] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[245] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[246] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[247] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[248] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[249] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[250] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[251] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[252] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[253] = &Parser::Act253;
#else
    void Act253();
#endif

#ifndef HEADERS
    rule_action[254] = &Parser::MakeLabeledStatement;
#endif

#ifndef HEADERS
    rule_action[255] = &Parser::MakeLabeledStatement;
#endif

#ifndef HEADERS
    rule_action[256] = &Parser::Act256;
#else
    void Act256();
#endif

#ifndef HEADERS
    rule_action[257] = &Parser::MakeExpressionStatement;
#endif

#ifndef HEADERS
    rule_action[258] = &Parser::MakeExpressionStatement;
#endif

#ifndef HEADERS
    rule_action[259] = &Parser::MakeExpressionStatement;
#endif

#ifndef HEADERS
    rule_action[260] = &Parser::MakeExpressionStatement;
#endif

#ifndef HEADERS
    rule_action[261] = &Parser::MakeExpressionStatement;
#endif

#ifndef HEADERS
    rule_action[262] = &Parser::MakeExpressionStatement;
#endif

#ifndef HEADERS
    rule_action[263] = &Parser::MakeExpressionStatement;
#endif

#ifndef HEADERS
    rule_action[264] = &Parser::MakeIfThenElseStatement;
#endif

#ifndef HEADERS
    rule_action[265] = &Parser::MakeIfThenElseStatement;
#endif

#ifndef HEADERS
    rule_action[266] = &Parser::MakeIfThenElseStatement;
#endif

#ifndef HEADERS
    rule_action[267] = &Parser::Act267;
#else
    void Act267();
#endif

#ifndef HEADERS
    rule_action[268] = &Parser::Act268;
#else
    void Act268();
#endif

#ifndef HEADERS
    rule_action[269] = &Parser::Act269;
#else
    void Act269();
#endif

#ifndef HEADERS
    rule_action[270] = &Parser::StartList;
#endif

#ifndef HEADERS
    rule_action[271] = &Parser::AddList2;
#endif

#ifndef HEADERS
    rule_action[272] = &Parser::Act272;
#else
    void Act272();
#endif

#ifndef HEADERS
    rule_action[273] = &Parser::StartList;
#endif

#ifndef HEADERS
    rule_action[274] = &Parser::AddList2;
#endif

#ifndef HEADERS
    rule_action[275] = &Parser::NullAction;
#endif

#ifndef HEADERS
    rule_action[276] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[277] = &Parser::MakeSwitchLabel;
#endif

#ifndef HEADERS
    rule_action[278] = &Parser::MakeSwitchLabel;
#endif

#ifndef HEADERS
    rule_action[279] = &Parser::MakeWhileStatement;
#endif

#ifndef HEADERS
    rule_action[280] = &Parser::MakeWhileStatement;
#endif

#ifndef HEADERS
    rule_action[281] = &Parser::Act281;
#else
    void Act281();
#endif

#ifndef HEADERS
    rule_action[282] = &Parser::MakeForStatement;
#endif

#ifndef HEADERS
    rule_action[283] = &Parser::MakeForStatement;
#endif

#ifndef HEADERS
    rule_action[284] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[285] = &Parser::StartList;
#endif

#ifndef HEADERS
    rule_action[286] = &Parser::NullAction;
#endif

#ifndef HEADERS
    rule_action[287] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[288] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[289] = &Parser::NullAction;
#endif

#ifndef HEADERS
    rule_action[290] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[291] = &Parser::StartList;
#endif

#ifndef HEADERS
    rule_action[292] = &Parser::AddList3;
#endif

#ifndef HEADERS
    rule_action[293] = &Parser::MakeForeachStatement;
#endif

#ifndef HEADERS
    rule_action[294] = &Parser::MakeForeachStatement;
#endif

#ifndef HEADERS
    rule_action[295] = &Parser::MakeAssertStatement;
#endif

#ifndef HEADERS
    rule_action[296] = &Parser::MakeAssertStatement;
#endif

#ifndef HEADERS
    rule_action[297] = &Parser::Act297;
#else
    void Act297();
#endif

#ifndef HEADERS
    rule_action[298] = &Parser::Act298;
#else
    void Act298();
#endif

#ifndef HEADERS
    rule_action[299] = &Parser::Act299;
#else
    void Act299();
#endif

#ifndef HEADERS
    rule_action[300] = &Parser::Act300;
#else
    void Act300();
#endif

#ifndef HEADERS
    rule_action[301] = &Parser::Act301;
#else
    void Act301();
#endif

#ifndef HEADERS
    rule_action[302] = &Parser::MakeTryStatement;
#endif

#ifndef HEADERS
    rule_action[303] = &Parser::MakeTryStatement;
#endif

#ifndef HEADERS
    rule_action[304] = &Parser::StartList;
#endif

#ifndef HEADERS
    rule_action[305] = &Parser::AddList2;
#endif

#ifndef HEADERS
    rule_action[306] = &Parser::NullAction;
#endif

#ifndef HEADERS
    rule_action[307] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[308] = &Parser::Act308;
#else
    void Act308();
#endif

#ifndef HEADERS
    rule_action[309] = &Parser::Act309;
#else
    void Act309();
#endif

#ifndef HEADERS
    rule_action[310] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[311] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[312] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[313] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[314] = &Parser::Act314;
#else
    void Act314();
#endif

#ifndef HEADERS
    rule_action[315] = &Parser::MakeParenthesizedExpression;
#endif

#ifndef HEADERS
    rule_action[316] = &Parser::MakeParenthesizedExpression;
#endif

#ifndef HEADERS
    rule_action[317] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[318] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[319] = &Parser::Act319;
#else
    void Act319();
#endif

#ifndef HEADERS
    rule_action[320] = &Parser::MakeClassLiteral;
#endif

#ifndef HEADERS
    rule_action[321] = &Parser::MakeClassLiteral;
#endif

#ifndef HEADERS
    rule_action[322] = &Parser::MakeClassLiteral;
#endif

#ifndef HEADERS
    rule_action[323] = &Parser::MakeClassLiteral;
#endif

#ifndef HEADERS
    rule_action[324] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[325] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[326] = &Parser::Act326;
#else
    void Act326();
#endif

#ifndef HEADERS
    rule_action[327] = &Parser::Act327;
#else
    void Act327();
#endif

#ifndef HEADERS
    rule_action[328] = &Parser::MakeQualifiedNew;
#endif

#ifndef HEADERS
    rule_action[329] = &Parser::MakeQualifiedNew;
#endif

#ifndef HEADERS
    rule_action[330] = &Parser::StartList;
#endif

#ifndef HEADERS
    rule_action[331] = &Parser::AddList3;
#endif

#ifndef HEADERS
    rule_action[332] = &Parser::NullAction;
#endif

#ifndef HEADERS
    rule_action[333] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[334] = &Parser::MakeArrayCreationUninitialized;
#endif

#ifndef HEADERS
    rule_action[335] = &Parser::MakeArrayCreationUninitialized;
#endif

#ifndef HEADERS
    rule_action[336] = &Parser::MakeArrayCreationInitialized;
#endif

#ifndef HEADERS
    rule_action[337] = &Parser::MakeArrayCreationInitialized;
#endif

#ifndef HEADERS
    rule_action[338] = &Parser::StartList;
#endif

#ifndef HEADERS
    rule_action[339] = &Parser::AddList2;
#endif

#ifndef HEADERS
    rule_action[340] = &Parser::Act340;
#else
    void Act340();
#endif

#ifndef HEADERS
    rule_action[341] = &Parser::Act341;
#else
    void Act341();
#endif

#ifndef HEADERS
    rule_action[342] = &Parser::Act342;
#else
    void Act342();
#endif

#ifndef HEADERS
    rule_action[343] = &Parser::NullAction;
#endif

#ifndef HEADERS
    rule_action[344] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[345] = &Parser::Act345;
#else
    void Act345();
#endif

#ifndef HEADERS
    rule_action[346] = &Parser::Act346;
#else
    void Act346();
#endif

#ifndef HEADERS
    rule_action[347] = &Parser::MakeFieldAccess;
#endif

#ifndef HEADERS
    rule_action[348] = &Parser::MakeFieldAccess;
#endif

#ifndef HEADERS
    rule_action[349] = &Parser::Act349;
#else
    void Act349();
#endif

#ifndef HEADERS
    rule_action[350] = &Parser::MakeMethodInvocation;
#endif

#ifndef HEADERS
    rule_action[351] = &Parser::MakeMethodInvocation;
#endif

#ifndef HEADERS
    rule_action[352] = &Parser::MakeMethodInvocation;
#endif

#ifndef HEADERS
    rule_action[353] = &Parser::MakeMethodInvocation;
#endif

#ifndef HEADERS
    rule_action[354] = &Parser::MakeMethodInvocation;
#endif

#ifndef HEADERS
    rule_action[355] = &Parser::MakeMethodInvocation;
#endif

#ifndef HEADERS
    rule_action[356] = &Parser::MakeArrayAccess;
#endif

#ifndef HEADERS
    rule_action[357] = &Parser::MakeArrayAccess;
#endif

#ifndef HEADERS
    rule_action[358] = &Parser::MakeArrayAccess;
#endif

#ifndef HEADERS
    rule_action[359] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[360] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[361] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[362] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[363] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[364] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[365] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[366] = &Parser::Act366;
#else
    void Act366();
#endif

#ifndef HEADERS
    rule_action[367] = &Parser::Act367;
#else
    void Act367();
#endif

#ifndef HEADERS
    rule_action[368] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[369] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[370] = &Parser::MakePreUnaryExpression;
#endif

#ifndef HEADERS
    rule_action[371] = &Parser::MakePreUnaryExpression;
#endif

#ifndef HEADERS
    rule_action[372] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[373] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[374] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[375] = &Parser::MakePreUnaryExpression;
#endif

#ifndef HEADERS
    rule_action[376] = &Parser::MakePreUnaryExpression;
#endif

#ifndef HEADERS
    rule_action[377] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[378] = &Parser::MakePreUnaryExpression;
#endif

#ifndef HEADERS
    rule_action[379] = &Parser::MakePreUnaryExpression;
#endif

#ifndef HEADERS
    rule_action[380] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[381] = &Parser::MakePreUnaryExpression;
#endif

#ifndef HEADERS
    rule_action[382] = &Parser::MakePreUnaryExpression;
#endif

#ifndef HEADERS
    rule_action[383] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[384] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[385] = &Parser::MakePreUnaryExpression;
#endif

#ifndef HEADERS
    rule_action[386] = &Parser::MakePreUnaryExpression;
#endif

#ifndef HEADERS
    rule_action[387] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[388] = &Parser::MakeCastExpression;
#endif

#ifndef HEADERS
    rule_action[389] = &Parser::MakeCastExpression;
#endif

#ifndef HEADERS
    rule_action[390] = &Parser::MakeCastExpression;
#endif

#ifndef HEADERS
    rule_action[391] = &Parser::Act391;
#else
    void Act391();
#endif

#ifndef HEADERS
    rule_action[392] = &Parser::Act392;
#else
    void Act392();
#endif

#ifndef HEADERS
    rule_action[393] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[394] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[395] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[396] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[397] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[398] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[399] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[400] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[401] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[402] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[403] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[404] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[405] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[406] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[407] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[408] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[409] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[410] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[411] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[412] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[413] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[414] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[415] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[416] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[417] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[418] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[419] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[420] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[421] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[422] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[423] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[424] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[425] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[426] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[427] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[428] = &Parser::MakeInstanceofExpression;
#endif

#ifndef HEADERS
    rule_action[429] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[430] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[431] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[432] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[433] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[434] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[435] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[436] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[437] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[438] = &Parser::MakeInstanceofExpression;
#endif

#ifndef HEADERS
    rule_action[439] = &Parser::MakeInstanceofExpression;
#endif

#ifndef HEADERS
    rule_action[440] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[441] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[442] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[443] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[444] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[445] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[446] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[447] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[448] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[449] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[450] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[451] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[452] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[453] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[454] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[455] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[456] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[457] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[458] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[459] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[460] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[461] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[462] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[463] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[464] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[465] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[466] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[467] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[468] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[469] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[470] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[471] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[472] = &Parser::MakeBinaryExpression;
#endif

#ifndef HEADERS
    rule_action[473] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[474] = &Parser::MakeConditionalExpression;
#endif

#ifndef HEADERS
    rule_action[475] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[476] = &Parser::MakeConditionalExpression;
#endif

#ifndef HEADERS
    rule_action[477] = &Parser::MakeConditionalExpression;
#endif

#ifndef HEADERS
    rule_action[478] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[479] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[480] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[481] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[482] = &Parser::Act482;
#else
    void Act482();
#endif

#ifndef HEADERS
    rule_action[483] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[484] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[485] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[486] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[487] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[488] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[489] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[490] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[491] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[492] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[493] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[494] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[495] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[496] = &Parser::NullAction;
#endif

#ifndef HEADERS
    rule_action[497] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[498] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[499] = &Parser::NullAction;
#endif

#ifndef HEADERS
    rule_action[500] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[501] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[502] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[503] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[504] = &Parser::Act504;
#else
    void Act504();
#endif

#ifndef HEADERS
    rule_action[505] = &Parser::Act505;
#else
    void Act505();
#endif

#ifndef HEADERS
    rule_action[506] = &Parser::SetSym1ToSym2;
#endif

#ifndef HEADERS
    rule_action[507] = &Parser::NullAction;
#endif

#ifndef HEADERS
    rule_action[508] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[509] = &Parser::MakeWildcard;
#endif

#ifndef HEADERS
    rule_action[510] = &Parser::MakeWildcard;
#endif

#ifndef HEADERS
    rule_action[511] = &Parser::MakeWildcard;
#endif

#ifndef HEADERS
    rule_action[512] = &Parser::MakeWildcard;
#endif

#ifndef HEADERS
    rule_action[513] = &Parser::MakeWildcard;
#endif

#ifndef HEADERS
    rule_action[514] = &Parser::MakeWildcard;
#endif

#ifndef HEADERS
    rule_action[515] = &Parser::MakeWildcard;
#endif

#ifndef HEADERS
    rule_action[516] = &Parser::MakeWildcard;
#endif

#ifndef HEADERS
    rule_action[517] = &Parser::MakeWildcard;
#endif

#ifndef HEADERS
    rule_action[518] = &Parser::MakeWildcard;
#endif

#ifndef HEADERS
    rule_action[519] = &Parser::MakeWildcard;
#endif

#ifndef HEADERS
    rule_action[520] = &Parser::MakeWildcard;
#endif

#ifndef HEADERS
    rule_action[521] = &Parser::StartList;
#endif

#ifndef HEADERS
    rule_action[522] = &Parser::AddList3;
#endif

#ifndef HEADERS
    rule_action[523] = &Parser::StartList;
#endif

#ifndef HEADERS
    rule_action[524] = &Parser::AddList3;
#endif

#ifndef HEADERS
    rule_action[525] = &Parser::StartList;
#endif

#ifndef HEADERS
    rule_action[526] = &Parser::AddList3;
#endif

#ifndef HEADERS
    rule_action[527] = &Parser::StartList;
#endif

#ifndef HEADERS
    rule_action[528] = &Parser::AddList3;
#endif

#ifndef HEADERS
    rule_action[529] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[530] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[531] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[532] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[533] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[534] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[535] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[536] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[537] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[538] = &Parser::MakeTypeArguments;
#endif

#ifndef HEADERS
    rule_action[539] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[540] = &Parser::MakeTypeArguments;
#endif

#ifndef HEADERS
    rule_action[541] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[542] = &Parser::SetSym1ToSym2;
#endif

#ifndef HEADERS
    rule_action[543] = &Parser::NullAction;
#endif

#ifndef HEADERS
    rule_action[544] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[545] = &Parser::StartList;
#endif

#ifndef HEADERS
    rule_action[546] = &Parser::AddList3;
#endif

#ifndef HEADERS
    rule_action[547] = &Parser::StartList;
#endif

#ifndef HEADERS
    rule_action[548] = &Parser::AddList3;
#endif

#ifndef HEADERS
    rule_action[549] = &Parser::MakeTypeParameter;
#endif

#ifndef HEADERS
    rule_action[550] = &Parser::MakeTypeParameter;
#endif

#ifndef HEADERS
    rule_action[551] = &Parser::MakeTypeParameter;
#endif

#ifndef HEADERS
    rule_action[552] = &Parser::MakeTypeBound;
#endif

#ifndef HEADERS
    rule_action[553] = &Parser::NullAction;
#endif

#ifndef HEADERS
    rule_action[554] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[555] = &Parser::MakeTypeBound;
#endif

#ifndef HEADERS
    rule_action[556] = &Parser::MakeTypeBound;
#endif

#ifndef HEADERS
    rule_action[557] = &Parser::StartList;
#endif

#ifndef HEADERS
    rule_action[558] = &Parser::AddList2;
#endif

#ifndef HEADERS
    rule_action[559] = &Parser::NullAction;
#endif

#ifndef HEADERS
    rule_action[560] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[561] = &Parser::StartList;
#endif

#ifndef HEADERS
    rule_action[562] = &Parser::AddList2;
#endif

#ifndef HEADERS
    rule_action[563] = &Parser::SetSym1ToSym2;
#endif

#ifndef HEADERS
    rule_action[564] = &Parser::SetSym1ToSym2;
#endif

#ifndef HEADERS
    rule_action[565] = &Parser::NoAction;
#endif

#ifndef HEADERS
    rule_action[566] = &Parser::MakeTypeArguments;
#endif

// ==========================================================================
// Java 7 Rules (567-583)
// ==========================================================================

// Rule 567: TryStatement ::= 'try' ResourceSpecification Block Catchesopt Marker
#ifndef HEADERS
    rule_action[567] = &Parser::MakeTryWithResources;
#else
    void MakeTryWithResources();
#endif

// Rule 568: TryStatement ::= 'try' ResourceSpecification Block Catchesopt Finally
#ifndef HEADERS
    rule_action[568] = &Parser::MakeTryWithResources;
#endif

// Rule 569: ResourceSpecification ::= '(' Resources Marker ')'
#ifndef HEADERS
    rule_action[569] = &Parser::SetSym1ToSym2;
#endif

// Rule 570: ResourceSpecification ::= '(' Resources ';' ')'
#ifndef HEADERS
    rule_action[570] = &Parser::SetSym1ToSym2;
#endif

// Rule 571: Resources ::= Resource
#ifndef HEADERS
    rule_action[571] = &Parser::StartList;
#endif

// Rule 572: Resources ::= Resources ';' Resource
#ifndef HEADERS
    rule_action[572] = &Parser::AddList3;
#endif

// Rule 573: Resource ::= Type VariableDeclaratorId '=' Expression
#ifndef HEADERS
    rule_action[573] = &Parser::MakeResource;
#else
    void MakeResource();
#endif

// Rule 574: Resource ::= Modifiers Type VariableDeclaratorId '=' Expression
#ifndef HEADERS
    rule_action[574] = &Parser::MakeResource;
#endif

// Rule 575: CatchClause ::= 'catch' '(' UnionType VariableDeclaratorId ')' Block
#ifndef HEADERS
    rule_action[575] = &Parser::MakeMultiCatch;
#else
    void MakeMultiCatch();
#endif

// Rule 576: CatchClause ::= 'catch' '(' Modifiers UnionType VariableDeclaratorId ')' Block
#ifndef HEADERS
    rule_action[576] = &Parser::MakeMultiCatch;
#endif

// Rule 577: UnionType ::= ClassOrInterfaceType '|' ClassOrInterfaceType
#ifndef HEADERS
    rule_action[577] = &Parser::MakeUnionType;
#else
    void MakeUnionType();
#endif

// Rule 578: UnionType ::= UnionType '|' ClassOrInterfaceType
#ifndef HEADERS
    rule_action[578] = &Parser::MakeUnionType;
#endif

// Rule 579: ClassOrInterfaceTypePlain ::= ClassOrInterface
#ifndef HEADERS
    rule_action[579] = &Parser::NoAction;
#endif

// Rule 580: ClassOrInterfaceTypeParameterized ::= ClassOrInterface '<' TypeArgumentList1 Marker
#ifndef HEADERS
    rule_action[580] = &Parser::MakeTypeArguments;
#endif

// Rule 581: ClassOrInterfaceTypeParameterized ::= ClassOrInterface '<' '>' (Diamond)
#ifndef HEADERS
    rule_action[581] = &Parser::MakeDiamondType;
#else
    void MakeDiamondType();
#endif

#ifndef HEADERS
    return;
}


} // Close namespace Jopa block

#endif // ! HEADERS
