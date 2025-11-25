#include "ast_serializer.h"
#include "ast.h"
#include "stream.h"
#include <cstring>

namespace Jopa {

AstSerializer::AstSerializer(LexStream& lex_stream)
    : lex_stream_(lex_stream)
{
}

nlohmann::json AstSerializer::tokenLocation(TokenIndex token) {
    if (token == 0) {
        return nlohmann::json::object();
    }
    return {
        {"line", lex_stream_.Line(token)},
        {"column", lex_stream_.Column(token)}
    };
}

std::string AstSerializer::tokenText(TokenIndex token) {
    if (token == 0) {
        return "";
    }
    const wchar_t* name = lex_stream_.NameString(token);
    unsigned len = lex_stream_.NameStringLength(token);
    std::string result;
    result.reserve(len);
    for (unsigned i = 0; i < len; ++i) {
        result.push_back(static_cast<char>(name[i]));
    }
    return result;
}

nlohmann::json AstSerializer::serialize(AstCompilationUnit* unit) {
    if (!unit) {
        return nlohmann::json::object();
    }
    return serializeCompilationUnit(unit);
}

nlohmann::json AstSerializer::serialize(AstPackageDeclaration* decl) {
    if (!decl) {
        return nlohmann::json::object();
    }
    return serializePackageDeclaration(decl);
}

nlohmann::json AstSerializer::serializeAst(Ast* node) {
    if (!node) {
        return nullptr;
    }

    switch (node->kind) {
        case Ast::NAME:
            return serializeName(static_cast<AstName*>(node));
        case Ast::BLOCK:
            return serializeBlock(static_cast<AstBlock*>(node));
        case Ast::INT:
        case Ast::DOUBLE:
        case Ast::CHAR:
        case Ast::LONG:
        case Ast::FLOAT:
        case Ast::BYTE:
        case Ast::SHORT:
        case Ast::BOOLEAN:
        case Ast::VOID_TYPE:
            return serializePrimitiveType(static_cast<AstPrimitiveType*>(node));
        case Ast::BRACKETS:
            return serializeBrackets(static_cast<AstBrackets*>(node));
        case Ast::ARRAY:
            return serializeArrayType(static_cast<AstArrayType*>(node));
        case Ast::WILDCARD:
            return serializeWildcard(static_cast<AstWildcard*>(node));
        case Ast::TYPE_ARGUMENTS:
            return serializeTypeArguments(static_cast<AstTypeArguments*>(node));
        case Ast::TYPE:
            return serializeTypeName(static_cast<AstTypeName*>(node));
        case Ast::MEMBER_VALUE_PAIR:
            return serializeMemberValuePair(static_cast<AstMemberValuePair*>(node));
        case Ast::ANNOTATION:
            return serializeAnnotation(static_cast<AstAnnotation*>(node));
        case Ast::MODIFIER_KEYWORD:
            return serializeModifierKeyword(static_cast<AstModifierKeyword*>(node));
        case Ast::MODIFIERS:
            return serializeModifiers(static_cast<AstModifiers*>(node));
        case Ast::PACKAGE:
            return serializePackageDeclaration(static_cast<AstPackageDeclaration*>(node));
        case Ast::IMPORT:
            return serializeImportDeclaration(static_cast<AstImportDeclaration*>(node));
        case Ast::COMPILATION:
            return serializeCompilationUnit(static_cast<AstCompilationUnit*>(node));
        case Ast::EMPTY_DECLARATION:
            return serializeEmptyDeclaration(static_cast<AstEmptyDeclaration*>(node));
        case Ast::CLASS_BODY:
            return serializeClassBody(static_cast<AstClassBody*>(node));
        case Ast::TYPE_PARAM:
            return serializeTypeParameter(static_cast<AstTypeParameter*>(node));
        case Ast::PARAM_LIST:
            return serializeTypeParameters(static_cast<AstTypeParameters*>(node));
        case Ast::CLASS:
            return serializeClassDeclaration(static_cast<AstClassDeclaration*>(node));
        case Ast::ARRAY_INITIALIZER:
            return serializeArrayInitializer(static_cast<AstArrayInitializer*>(node));
        case Ast::VARIABLE_DECLARATOR_NAME:
            return serializeVariableDeclaratorId(static_cast<AstVariableDeclaratorId*>(node));
        case Ast::VARIABLE_DECLARATOR:
            return serializeVariableDeclarator(static_cast<AstVariableDeclarator*>(node));
        case Ast::FIELD:
            return serializeFieldDeclaration(static_cast<AstFieldDeclaration*>(node));
        case Ast::PARAMETER:
            return serializeFormalParameter(static_cast<AstFormalParameter*>(node));
        case Ast::METHOD_DECLARATOR:
            return serializeMethodDeclarator(static_cast<AstMethodDeclarator*>(node));
        case Ast::METHOD_BODY:
            return serializeMethodBody(static_cast<AstMethodBody*>(node));
        case Ast::METHOD:
            return serializeMethodDeclaration(static_cast<AstMethodDeclaration*>(node));
        case Ast::INITIALIZER:
            return serializeInitializerDeclaration(static_cast<AstInitializerDeclaration*>(node));
        case Ast::ARGUMENTS:
            return serializeArguments(static_cast<AstArguments*>(node));
        case Ast::THIS_CALL:
            return serializeThisCall(static_cast<AstThisCall*>(node));
        case Ast::SUPER_CALL:
            return serializeSuperCall(static_cast<AstSuperCall*>(node));
        case Ast::CONSTRUCTOR:
            return serializeConstructorDeclaration(static_cast<AstConstructorDeclaration*>(node));
        case Ast::ENUM_TYPE:
            return serializeEnumDeclaration(static_cast<AstEnumDeclaration*>(node));
        case Ast::ENUM:
            return serializeEnumConstant(static_cast<AstEnumConstant*>(node));
        case Ast::INTERFACE:
            return serializeInterfaceDeclaration(static_cast<AstInterfaceDeclaration*>(node));
        case Ast::ANNOTATION_TYPE:
            return serializeAnnotationDeclaration(static_cast<AstAnnotationDeclaration*>(node));
        case Ast::LOCAL_VARIABLE_DECLARATION:
            return serializeLocalVariableStatement(static_cast<AstLocalVariableStatement*>(node));
        case Ast::LOCAL_CLASS:
            return serializeLocalClassStatement(static_cast<AstLocalClassStatement*>(node));
        case Ast::IF:
            return serializeIfStatement(static_cast<AstIfStatement*>(node));
        case Ast::EMPTY_STATEMENT:
            return serializeEmptyStatement(static_cast<AstEmptyStatement*>(node));
        case Ast::EXPRESSION_STATEMENT:
            return serializeExpressionStatement(static_cast<AstExpressionStatement*>(node));
        case Ast::SWITCH_LABEL:
            return serializeSwitchLabel(static_cast<AstSwitchLabel*>(node));
        case Ast::SWITCH_BLOCK:
            return serializeSwitchBlockStatement(static_cast<AstSwitchBlockStatement*>(node));
        case Ast::SWITCH:
            return serializeSwitchStatement(static_cast<AstSwitchStatement*>(node));
        case Ast::WHILE:
            return serializeWhileStatement(static_cast<AstWhileStatement*>(node));
        case Ast::DO:
            return serializeDoStatement(static_cast<AstDoStatement*>(node));
        case Ast::FOR:
            return serializeForStatement(static_cast<AstForStatement*>(node));
        case Ast::FOREACH:
            return serializeForeachStatement(static_cast<AstForeachStatement*>(node));
        case Ast::BREAK:
            return serializeBreakStatement(static_cast<AstBreakStatement*>(node));
        case Ast::CONTINUE:
            return serializeContinueStatement(static_cast<AstContinueStatement*>(node));
        case Ast::RETURN:
            return serializeReturnStatement(static_cast<AstReturnStatement*>(node));
        case Ast::THROW:
            return serializeThrowStatement(static_cast<AstThrowStatement*>(node));
        case Ast::SYNCHRONIZED_STATEMENT:
            return serializeSynchronizedStatement(static_cast<AstSynchronizedStatement*>(node));
        case Ast::ASSERT:
            return serializeAssertStatement(static_cast<AstAssertStatement*>(node));
        case Ast::CATCH:
            return serializeCatchClause(static_cast<AstCatchClause*>(node));
        case Ast::FINALLY:
            return serializeFinallyClause(static_cast<AstFinallyClause*>(node));
        case Ast::TRY:
            return serializeTryStatement(static_cast<AstTryStatement*>(node));
        case Ast::INTEGER_LITERAL:
            return serializeIntegerLiteral(static_cast<AstIntegerLiteral*>(node));
        case Ast::LONG_LITERAL:
            return serializeLongLiteral(static_cast<AstLongLiteral*>(node));
        case Ast::FLOAT_LITERAL:
            return serializeFloatLiteral(static_cast<AstFloatLiteral*>(node));
        case Ast::DOUBLE_LITERAL:
            return serializeDoubleLiteral(static_cast<AstDoubleLiteral*>(node));
        case Ast::TRUE_LITERAL:
            return serializeTrueLiteral(static_cast<AstTrueLiteral*>(node));
        case Ast::FALSE_LITERAL:
            return serializeFalseLiteral(static_cast<AstFalseLiteral*>(node));
        case Ast::STRING_LITERAL:
            return serializeStringLiteral(static_cast<AstStringLiteral*>(node));
        case Ast::CHARACTER_LITERAL:
            return serializeCharacterLiteral(static_cast<AstCharacterLiteral*>(node));
        case Ast::NULL_LITERAL:
            return serializeNullLiteral(static_cast<AstNullLiteral*>(node));
        case Ast::CLASS_LITERAL:
            return serializeClassLiteral(static_cast<AstClassLiteral*>(node));
        case Ast::THIS_EXPRESSION:
            return serializeThisExpression(static_cast<AstThisExpression*>(node));
        case Ast::SUPER_EXPRESSION:
            return serializeSuperExpression(static_cast<AstSuperExpression*>(node));
        case Ast::PARENTHESIZED_EXPRESSION:
            return serializeParenthesizedExpression(static_cast<AstParenthesizedExpression*>(node));
        case Ast::CLASS_CREATION:
            return serializeClassCreationExpression(static_cast<AstClassCreationExpression*>(node));
        case Ast::DIM:
            return serializeDimExpr(static_cast<AstDimExpr*>(node));
        case Ast::ARRAY_CREATION:
            return serializeArrayCreationExpression(static_cast<AstArrayCreationExpression*>(node));
        case Ast::DOT:
            return serializeFieldAccess(static_cast<AstFieldAccess*>(node));
        case Ast::CALL:
            return serializeMethodInvocation(static_cast<AstMethodInvocation*>(node));
        case Ast::ARRAY_ACCESS:
            return serializeArrayAccess(static_cast<AstArrayAccess*>(node));
        case Ast::POST_UNARY:
            return serializePostUnaryExpression(static_cast<AstPostUnaryExpression*>(node));
        case Ast::PRE_UNARY:
            return serializePreUnaryExpression(static_cast<AstPreUnaryExpression*>(node));
        case Ast::CAST:
            return serializeCastExpression(static_cast<AstCastExpression*>(node));
        case Ast::BINARY:
            return serializeBinaryExpression(static_cast<AstBinaryExpression*>(node));
        case Ast::INSTANCEOF:
            return serializeInstanceofExpression(static_cast<AstInstanceofExpression*>(node));
        case Ast::CONDITIONAL:
            return serializeConditionalExpression(static_cast<AstConditionalExpression*>(node));
        case Ast::ASSIGNMENT:
            return serializeAssignmentExpression(static_cast<AstAssignmentExpression*>(node));
        default:
            return {{"kind", "unknown"}, {"kind_id", static_cast<int>(node->kind)}};
    }
}

nlohmann::json AstSerializer::serializeStatement(AstStatement* stmt) {
    return serializeAst(stmt);
}

nlohmann::json AstSerializer::serializeExpression(AstExpression* expr) {
    return serializeAst(expr);
}

nlohmann::json AstSerializer::serializeType(AstType* type) {
    return serializeAst(type);
}

nlohmann::json AstSerializer::serializeMemberValue(AstMemberValue* value) {
    return serializeAst(value);
}

nlohmann::json AstSerializer::serializeDeclared(AstDeclared* decl) {
    return serializeAst(decl);
}

nlohmann::json AstSerializer::serializeDeclaredType(AstDeclaredType* decl) {
    return serializeAst(decl);
}

nlohmann::json AstSerializer::serializeBlock(AstBlock* block) {
    if (!block) return nullptr;
    nlohmann::json statements = nlohmann::json::array();
    for (unsigned i = 0; i < block->NumStatements(); ++i) {
        statements.push_back(serializeStatement(block->Statement(i)));
    }
    return {
        {"kind", "Block"},
        {"location", tokenLocation(block->LeftToken())},
        {"statements", statements}
    };
}

nlohmann::json AstSerializer::serializeName(AstName* name) {
    if (!name) return nullptr;
    nlohmann::json result = {
        {"kind", "Name"},
        {"identifier", tokenText(name->identifier_token)},
        {"location", tokenLocation(name->identifier_token)}
    };
    if (name->base_opt) {
        result["base"] = serializeName(name->base_opt);
    }
    return result;
}

nlohmann::json AstSerializer::serializePrimitiveType(AstPrimitiveType* type) {
    if (!type) return nullptr;
    std::string typeName;
    switch (type->kind) {
        case Ast::INT: typeName = "int"; break;
        case Ast::DOUBLE: typeName = "double"; break;
        case Ast::CHAR: typeName = "char"; break;
        case Ast::LONG: typeName = "long"; break;
        case Ast::FLOAT: typeName = "float"; break;
        case Ast::BYTE: typeName = "byte"; break;
        case Ast::SHORT: typeName = "short"; break;
        case Ast::BOOLEAN: typeName = "boolean"; break;
        case Ast::VOID_TYPE: typeName = "void"; break;
        default: typeName = "unknown"; break;
    }
    return {
        {"kind", "PrimitiveType"},
        {"type", typeName},
        {"location", tokenLocation(type->primitive_kind_token)}
    };
}

nlohmann::json AstSerializer::serializeBrackets(AstBrackets* brackets) {
    if (!brackets) return nullptr;
    return {
        {"kind", "Brackets"},
        {"dims", brackets->dims},
        {"location", tokenLocation(brackets->left_bracket_token)}
    };
}

nlohmann::json AstSerializer::serializeArrayType(AstArrayType* type) {
    if (!type) return nullptr;
    return {
        {"kind", "ArrayType"},
        {"elementType", serializeType(type->type)},
        {"brackets", serializeBrackets(type->brackets)},
        {"location", tokenLocation(type->LeftToken())}
    };
}

nlohmann::json AstSerializer::serializeWildcard(AstWildcard* wildcard) {
    if (!wildcard) return nullptr;
    nlohmann::json result = {
        {"kind", "Wildcard"},
        {"location", tokenLocation(wildcard->question_token)}
    };
    if (wildcard->extends_token_opt) {
        result["bound_kind"] = "extends";
    } else if (wildcard->super_token_opt) {
        result["bound_kind"] = "super";
    }
    if (wildcard->bounds_opt) {
        result["bounds"] = serializeType(wildcard->bounds_opt);
    }
    return result;
}

nlohmann::json AstSerializer::serializeTypeArguments(AstTypeArguments* args) {
    if (!args) return nullptr;
    nlohmann::json typeArgs = nlohmann::json::array();
    for (unsigned i = 0; i < args->NumTypeArguments(); ++i) {
        typeArgs.push_back(serializeType(args->TypeArgument(i)));
    }
    return {
        {"kind", "TypeArguments"},
        {"arguments", typeArgs},
        {"location", tokenLocation(args->left_angle_token)}
    };
}

nlohmann::json AstSerializer::serializeTypeName(AstTypeName* type) {
    if (!type) return nullptr;
    nlohmann::json result = {
        {"kind", "TypeName"},
        {"name", serializeName(type->name)},
        {"location", tokenLocation(type->LeftToken())}
    };
    if (type->base_opt) {
        result["base"] = serializeTypeName(type->base_opt);
    }
    if (type->type_arguments_opt) {
        result["type_arguments"] = serializeTypeArguments(type->type_arguments_opt);
    }
    return result;
}

nlohmann::json AstSerializer::serializeMemberValuePair(AstMemberValuePair* pair) {
    if (!pair) return nullptr;
    nlohmann::json result = {
        {"kind", "MemberValuePair"},
        {"value", serializeMemberValue(pair->member_value)}
    };
    if (pair->identifier_token_opt) {
        result["name"] = tokenText(pair->identifier_token_opt);
        result["location"] = tokenLocation(pair->identifier_token_opt);
    }
    return result;
}

nlohmann::json AstSerializer::serializeAnnotation(AstAnnotation* annotation) {
    if (!annotation) return nullptr;
    nlohmann::json pairs = nlohmann::json::array();
    for (unsigned i = 0; i < annotation->NumMemberValuePairs(); ++i) {
        pairs.push_back(serializeMemberValuePair(annotation->MemberValuePair(i)));
    }
    return {
        {"kind", "Annotation"},
        {"name", serializeName(annotation->name)},
        {"member_value_pairs", pairs},
        {"location", tokenLocation(annotation->at_token)}
    };
}

nlohmann::json AstSerializer::serializeModifierKeyword(AstModifierKeyword* keyword) {
    if (!keyword) return nullptr;
    return {
        {"kind", "ModifierKeyword"},
        {"modifier", tokenText(keyword->modifier_token)},
        {"location", tokenLocation(keyword->modifier_token)}
    };
}

nlohmann::json AstSerializer::serializeModifiers(AstModifiers* modifiers) {
    if (!modifiers) return nullptr;
    nlohmann::json mods = nlohmann::json::array();
    for (unsigned i = 0; i < modifiers->NumModifiers(); ++i) {
        mods.push_back(serializeAst(modifiers->Modifier(i)));
    }
    return {
        {"kind", "Modifiers"},
        {"modifiers", mods},
        {"location", tokenLocation(modifiers->LeftToken())}
    };
}

nlohmann::json AstSerializer::serializePackageDeclaration(AstPackageDeclaration* decl) {
    if (!decl) return nullptr;
    nlohmann::json result = {
        {"kind", "PackageDeclaration"},
        {"name", serializeName(decl->name)},
        {"location", tokenLocation(decl->package_token)}
    };
    if (decl->modifiers_opt) {
        result["modifiers"] = serializeModifiers(decl->modifiers_opt);
    }
    return result;
}

nlohmann::json AstSerializer::serializeImportDeclaration(AstImportDeclaration* decl) {
    if (!decl) return nullptr;
    return {
        {"kind", "ImportDeclaration"},
        {"name", serializeName(decl->name)},
        {"is_static", decl->static_token_opt != 0},
        {"is_on_demand", decl->star_token_opt != 0},
        {"location", tokenLocation(decl->import_token)}
    };
}

nlohmann::json AstSerializer::serializeCompilationUnit(AstCompilationUnit* unit) {
    if (!unit) return nullptr;

    nlohmann::json imports = nlohmann::json::array();
    for (unsigned i = 0; i < unit->NumImportDeclarations(); ++i) {
        imports.push_back(serializeImportDeclaration(unit->ImportDeclaration(i)));
    }

    nlohmann::json types = nlohmann::json::array();
    for (unsigned i = 0; i < unit->NumTypeDeclarations(); ++i) {
        types.push_back(serializeDeclaredType(unit->TypeDeclaration(i)));
    }

    nlohmann::json result = {
        {"kind", "CompilationUnit"},
        {"imports", imports},
        {"types", types}
    };

    if (unit->package_declaration_opt) {
        result["package"] = serializePackageDeclaration(unit->package_declaration_opt);
    }

    return result;
}

nlohmann::json AstSerializer::serializeEmptyDeclaration(AstEmptyDeclaration* decl) {
    if (!decl) return nullptr;
    return {
        {"kind", "EmptyDeclaration"},
        {"location", tokenLocation(decl->semicolon_token)}
    };
}

nlohmann::json AstSerializer::serializeClassBody(AstClassBody* body) {
    if (!body) return nullptr;

    nlohmann::json declarations = nlohmann::json::array();
    for (unsigned i = 0; i < body->NumClassBodyDeclarations(); ++i) {
        declarations.push_back(serializeDeclared(body->ClassBodyDeclaration(i)));
    }

    return {
        {"kind", "ClassBody"},
        {"declarations", declarations},
        {"location", tokenLocation(body->left_brace_token)}
    };
}

nlohmann::json AstSerializer::serializeTypeParameter(AstTypeParameter* param) {
    if (!param) return nullptr;

    nlohmann::json bounds = nlohmann::json::array();
    for (unsigned i = 0; i < param->NumBounds(); ++i) {
        bounds.push_back(serializeTypeName(param->Bound(i)));
    }

    return {
        {"kind", "TypeParameter"},
        {"name", tokenText(param->identifier_token)},
        {"bounds", bounds},
        {"location", tokenLocation(param->identifier_token)}
    };
}

nlohmann::json AstSerializer::serializeTypeParameters(AstTypeParameters* params) {
    if (!params) return nullptr;

    nlohmann::json parameters = nlohmann::json::array();
    for (unsigned i = 0; i < params->NumTypeParameters(); ++i) {
        parameters.push_back(serializeTypeParameter(params->TypeParameter(i)));
    }

    return {
        {"kind", "TypeParameters"},
        {"parameters", parameters},
        {"location", tokenLocation(params->left_angle_token)}
    };
}

nlohmann::json AstSerializer::serializeClassDeclaration(AstClassDeclaration* decl) {
    if (!decl) return nullptr;

    nlohmann::json interfaces = nlohmann::json::array();
    for (unsigned i = 0; i < decl->NumInterfaces(); ++i) {
        interfaces.push_back(serializeTypeName(decl->Interface(i)));
    }

    nlohmann::json result = {
        {"kind", "ClassDeclaration"},
        {"name", tokenText(decl->class_body->identifier_token)},
        {"interfaces", interfaces},
        {"body", serializeClassBody(decl->class_body)},
        {"location", tokenLocation(decl->class_token)}
    };

    if (decl->modifiers_opt) {
        result["modifiers"] = serializeModifiers(decl->modifiers_opt);
    }
    if (decl->type_parameters_opt) {
        result["type_parameters"] = serializeTypeParameters(decl->type_parameters_opt);
    }
    if (decl->super_opt) {
        result["superclass"] = serializeTypeName(decl->super_opt);
    }

    return result;
}

nlohmann::json AstSerializer::serializeArrayInitializer(AstArrayInitializer* init) {
    if (!init) return nullptr;

    nlohmann::json elements = nlohmann::json::array();
    for (unsigned i = 0; i < init->NumVariableInitializers(); ++i) {
        elements.push_back(serializeMemberValue(init->VariableInitializer(i)));
    }

    return {
        {"kind", "ArrayInitializer"},
        {"elements", elements},
        {"location", tokenLocation(init->left_brace_token)}
    };
}

nlohmann::json AstSerializer::serializeVariableDeclaratorId(AstVariableDeclaratorId* id) {
    if (!id) return nullptr;

    nlohmann::json result = {
        {"kind", "VariableDeclaratorId"},
        {"name", tokenText(id->identifier_token)},
        {"location", tokenLocation(id->identifier_token)}
    };

    if (id->brackets_opt) {
        result["brackets"] = serializeBrackets(id->brackets_opt);
    }

    return result;
}

nlohmann::json AstSerializer::serializeVariableDeclarator(AstVariableDeclarator* decl) {
    if (!decl) return nullptr;

    nlohmann::json result = {
        {"kind", "VariableDeclarator"},
        {"name", serializeVariableDeclaratorId(decl->variable_declarator_name)},
        {"location", tokenLocation(decl->LeftToken())}
    };

    if (decl->variable_initializer_opt) {
        result["initializer"] = serializeAst(decl->variable_initializer_opt);
    }

    return result;
}

nlohmann::json AstSerializer::serializeFieldDeclaration(AstFieldDeclaration* decl) {
    if (!decl) return nullptr;

    nlohmann::json declarators = nlohmann::json::array();
    for (unsigned i = 0; i < decl->NumVariableDeclarators(); ++i) {
        declarators.push_back(serializeVariableDeclarator(decl->VariableDeclarator(i)));
    }

    nlohmann::json result = {
        {"kind", "FieldDeclaration"},
        {"type", serializeType(decl->type)},
        {"declarators", declarators},
        {"location", tokenLocation(decl->LeftToken())}
    };

    if (decl->modifiers_opt) {
        result["modifiers"] = serializeModifiers(decl->modifiers_opt);
    }

    return result;
}

nlohmann::json AstSerializer::serializeFormalParameter(AstFormalParameter* param) {
    if (!param) return nullptr;

    nlohmann::json result = {
        {"kind", "FormalParameter"},
        {"type", serializeType(param->type)},
        {"declarator", serializeVariableDeclarator(param->formal_declarator)},
        {"is_vararg", param->ellipsis_token_opt != 0},
        {"location", tokenLocation(param->LeftToken())}
    };

    if (param->modifiers_opt) {
        result["modifiers"] = serializeModifiers(param->modifiers_opt);
    }

    return result;
}

nlohmann::json AstSerializer::serializeMethodDeclarator(AstMethodDeclarator* decl) {
    if (!decl) return nullptr;

    nlohmann::json params = nlohmann::json::array();
    for (unsigned i = 0; i < decl->NumFormalParameters(); ++i) {
        params.push_back(serializeFormalParameter(decl->FormalParameter(i)));
    }

    nlohmann::json result = {
        {"kind", "MethodDeclarator"},
        {"name", tokenText(decl->identifier_token)},
        {"parameters", params},
        {"location", tokenLocation(decl->identifier_token)}
    };

    if (decl->brackets_opt) {
        result["brackets"] = serializeBrackets(decl->brackets_opt);
    }

    return result;
}

nlohmann::json AstSerializer::serializeMethodBody(AstMethodBody* body) {
    if (!body) return nullptr;

    nlohmann::json statements = nlohmann::json::array();
    for (unsigned i = 0; i < body->NumStatements(); ++i) {
        statements.push_back(serializeStatement(body->Statement(i)));
    }

    nlohmann::json result = {
        {"kind", "MethodBody"},
        {"statements", statements},
        {"location", tokenLocation(body->LeftToken())}
    };

    if (body->explicit_constructor_opt) {
        result["explicit_constructor"] = serializeStatement(body->explicit_constructor_opt);
    }

    return result;
}

nlohmann::json AstSerializer::serializeMethodDeclaration(AstMethodDeclaration* decl) {
    if (!decl) return nullptr;

    nlohmann::json throws = nlohmann::json::array();
    for (unsigned i = 0; i < decl->NumThrows(); ++i) {
        throws.push_back(serializeTypeName(decl->Throw(i)));
    }

    nlohmann::json result = {
        {"kind", "MethodDeclaration"},
        {"return_type", serializeType(decl->type)},
        {"declarator", serializeMethodDeclarator(decl->method_declarator)},
        {"throws", throws},
        {"location", tokenLocation(decl->LeftToken())}
    };

    if (decl->modifiers_opt) {
        result["modifiers"] = serializeModifiers(decl->modifiers_opt);
    }
    if (decl->type_parameters_opt) {
        result["type_parameters"] = serializeTypeParameters(decl->type_parameters_opt);
    }
    if (decl->method_body_opt) {
        result["body"] = serializeMethodBody(decl->method_body_opt);
    }
    if (decl->default_value_opt) {
        result["default_value"] = serializeMemberValue(decl->default_value_opt);
    }

    return result;
}

nlohmann::json AstSerializer::serializeInitializerDeclaration(AstInitializerDeclaration* decl) {
    if (!decl) return nullptr;

    nlohmann::json result = {
        {"kind", "InitializerDeclaration"},
        {"body", serializeMethodBody(decl->block)},
        {"location", tokenLocation(decl->LeftToken())}
    };

    if (decl->modifiers_opt) {
        result["modifiers"] = serializeModifiers(decl->modifiers_opt);
    }

    return result;
}

nlohmann::json AstSerializer::serializeArguments(AstArguments* args) {
    if (!args) return nullptr;

    nlohmann::json arguments = nlohmann::json::array();
    for (unsigned i = 0; i < args->NumArguments(); ++i) {
        arguments.push_back(serializeExpression(args->Argument(i)));
    }

    return {
        {"kind", "Arguments"},
        {"arguments", arguments},
        {"location", tokenLocation(args->left_parenthesis_token)}
    };
}

nlohmann::json AstSerializer::serializeThisCall(AstThisCall* call) {
    if (!call) return nullptr;

    nlohmann::json result = {
        {"kind", "ThisCall"},
        {"arguments", serializeArguments(call->arguments)},
        {"location", tokenLocation(call->this_token)}
    };

    if (call->type_arguments_opt) {
        result["type_arguments"] = serializeTypeArguments(call->type_arguments_opt);
    }

    return result;
}

nlohmann::json AstSerializer::serializeSuperCall(AstSuperCall* call) {
    if (!call) return nullptr;

    nlohmann::json result = {
        {"kind", "SuperCall"},
        {"arguments", serializeArguments(call->arguments)},
        {"location", tokenLocation(call->super_token)}
    };

    if (call->base_opt) {
        result["base"] = serializeExpression(call->base_opt);
    }
    if (call->type_arguments_opt) {
        result["type_arguments"] = serializeTypeArguments(call->type_arguments_opt);
    }

    return result;
}

nlohmann::json AstSerializer::serializeConstructorDeclaration(AstConstructorDeclaration* decl) {
    if (!decl) return nullptr;

    nlohmann::json throws = nlohmann::json::array();
    for (unsigned i = 0; i < decl->NumThrows(); ++i) {
        throws.push_back(serializeTypeName(decl->Throw(i)));
    }

    nlohmann::json result = {
        {"kind", "ConstructorDeclaration"},
        {"declarator", serializeMethodDeclarator(decl->constructor_declarator)},
        {"throws", throws},
        {"body", serializeMethodBody(decl->constructor_body)},
        {"location", tokenLocation(decl->LeftToken())}
    };

    if (decl->modifiers_opt) {
        result["modifiers"] = serializeModifiers(decl->modifiers_opt);
    }
    if (decl->type_parameters_opt) {
        result["type_parameters"] = serializeTypeParameters(decl->type_parameters_opt);
    }

    return result;
}

nlohmann::json AstSerializer::serializeEnumDeclaration(AstEnumDeclaration* decl) {
    if (!decl) return nullptr;

    nlohmann::json interfaces = nlohmann::json::array();
    for (unsigned i = 0; i < decl->NumInterfaces(); ++i) {
        interfaces.push_back(serializeTypeName(decl->Interface(i)));
    }

    nlohmann::json constants = nlohmann::json::array();
    for (unsigned i = 0; i < decl->NumEnumConstants(); ++i) {
        constants.push_back(serializeEnumConstant(decl->EnumConstant(i)));
    }

    nlohmann::json result = {
        {"kind", "EnumDeclaration"},
        {"name", tokenText(decl->class_body->identifier_token)},
        {"interfaces", interfaces},
        {"constants", constants},
        {"body", serializeClassBody(decl->class_body)},
        {"location", tokenLocation(decl->enum_token)}
    };

    if (decl->modifiers_opt) {
        result["modifiers"] = serializeModifiers(decl->modifiers_opt);
    }

    return result;
}

nlohmann::json AstSerializer::serializeEnumConstant(AstEnumConstant* constant) {
    if (!constant) return nullptr;

    nlohmann::json result = {
        {"kind", "EnumConstant"},
        {"name", tokenText(constant->identifier_token)},
        {"location", tokenLocation(constant->identifier_token)}
    };

    if (constant->modifiers_opt) {
        result["modifiers"] = serializeModifiers(constant->modifiers_opt);
    }
    if (constant->arguments_opt) {
        result["arguments"] = serializeArguments(constant->arguments_opt);
    }
    if (constant->class_body_opt) {
        result["body"] = serializeClassBody(constant->class_body_opt);
    }

    return result;
}

nlohmann::json AstSerializer::serializeInterfaceDeclaration(AstInterfaceDeclaration* decl) {
    if (!decl) return nullptr;

    nlohmann::json interfaces = nlohmann::json::array();
    for (unsigned i = 0; i < decl->NumInterfaces(); ++i) {
        interfaces.push_back(serializeTypeName(decl->Interface(i)));
    }

    nlohmann::json result = {
        {"kind", "InterfaceDeclaration"},
        {"name", tokenText(decl->class_body->identifier_token)},
        {"extends", interfaces},
        {"body", serializeClassBody(decl->class_body)},
        {"location", tokenLocation(decl->interface_token)}
    };

    if (decl->modifiers_opt) {
        result["modifiers"] = serializeModifiers(decl->modifiers_opt);
    }
    if (decl->type_parameters_opt) {
        result["type_parameters"] = serializeTypeParameters(decl->type_parameters_opt);
    }

    return result;
}

nlohmann::json AstSerializer::serializeAnnotationDeclaration(AstAnnotationDeclaration* decl) {
    if (!decl) return nullptr;

    nlohmann::json result = {
        {"kind", "AnnotationDeclaration"},
        {"name", tokenText(decl->class_body->identifier_token)},
        {"body", serializeClassBody(decl->class_body)},
        {"location", tokenLocation(decl->interface_token)}
    };

    if (decl->modifiers_opt) {
        result["modifiers"] = serializeModifiers(decl->modifiers_opt);
    }

    return result;
}

nlohmann::json AstSerializer::serializeLocalVariableStatement(AstLocalVariableStatement* stmt) {
    if (!stmt) return nullptr;

    nlohmann::json declarators = nlohmann::json::array();
    for (unsigned i = 0; i < stmt->NumVariableDeclarators(); ++i) {
        declarators.push_back(serializeVariableDeclarator(stmt->VariableDeclarator(i)));
    }

    nlohmann::json result = {
        {"kind", "LocalVariableStatement"},
        {"type", serializeType(stmt->type)},
        {"declarators", declarators},
        {"location", tokenLocation(stmt->LeftToken())}
    };

    if (stmt->modifiers_opt) {
        result["modifiers"] = serializeModifiers(stmt->modifiers_opt);
    }

    return result;
}

nlohmann::json AstSerializer::serializeLocalClassStatement(AstLocalClassStatement* stmt) {
    if (!stmt) return nullptr;

    return {
        {"kind", "LocalClassStatement"},
        {"declaration", serializeDeclaredType(stmt->declaration)},
        {"location", tokenLocation(stmt->LeftToken())}
    };
}

nlohmann::json AstSerializer::serializeIfStatement(AstIfStatement* stmt) {
    if (!stmt) return nullptr;

    nlohmann::json result = {
        {"kind", "IfStatement"},
        {"condition", serializeExpression(stmt->expression)},
        {"then_statement", serializeStatement(stmt->true_statement)},
        {"location", tokenLocation(stmt->if_token)}
    };

    if (stmt->false_statement_opt) {
        result["else_statement"] = serializeStatement(stmt->false_statement_opt);
    }

    return result;
}

nlohmann::json AstSerializer::serializeEmptyStatement(AstEmptyStatement* stmt) {
    if (!stmt) return nullptr;
    return {
        {"kind", "EmptyStatement"},
        {"location", tokenLocation(stmt->semicolon_token)}
    };
}

nlohmann::json AstSerializer::serializeExpressionStatement(AstExpressionStatement* stmt) {
    if (!stmt) return nullptr;
    return {
        {"kind", "ExpressionStatement"},
        {"expression", serializeExpression(stmt->expression)},
        {"location", tokenLocation(stmt->LeftToken())}
    };
}

nlohmann::json AstSerializer::serializeSwitchLabel(AstSwitchLabel* label) {
    if (!label) return nullptr;

    nlohmann::json result = {
        {"kind", "SwitchLabel"},
        {"is_default", label->expression_opt == nullptr},
        {"location", tokenLocation(label->case_token)}
    };

    if (label->expression_opt) {
        result["expression"] = serializeExpression(label->expression_opt);
    }

    return result;
}

nlohmann::json AstSerializer::serializeSwitchBlockStatement(AstSwitchBlockStatement* stmt) {
    if (!stmt) return nullptr;

    nlohmann::json labels = nlohmann::json::array();
    for (unsigned i = 0; i < stmt->NumSwitchLabels(); ++i) {
        labels.push_back(serializeSwitchLabel(stmt->SwitchLabel(i)));
    }

    nlohmann::json statements = nlohmann::json::array();
    for (unsigned i = 0; i < stmt->NumStatements(); ++i) {
        statements.push_back(serializeStatement(stmt->Statement(i)));
    }

    return {
        {"kind", "SwitchBlockStatement"},
        {"labels", labels},
        {"statements", statements},
        {"location", tokenLocation(stmt->LeftToken())}
    };
}

nlohmann::json AstSerializer::serializeSwitchStatement(AstSwitchStatement* stmt) {
    if (!stmt) return nullptr;

    nlohmann::json blocks = nlohmann::json::array();
    for (unsigned i = 0; i < stmt->NumBlocks(); ++i) {
        blocks.push_back(serializeSwitchBlockStatement(stmt->Block(i)));
    }

    return {
        {"kind", "SwitchStatement"},
        {"expression", serializeExpression(stmt->expression)},
        {"blocks", blocks},
        {"location", tokenLocation(stmt->switch_token)}
    };
}

nlohmann::json AstSerializer::serializeWhileStatement(AstWhileStatement* stmt) {
    if (!stmt) return nullptr;
    return {
        {"kind", "WhileStatement"},
        {"condition", serializeExpression(stmt->expression)},
        {"body", serializeStatement(stmt->statement)},
        {"location", tokenLocation(stmt->while_token)}
    };
}

nlohmann::json AstSerializer::serializeDoStatement(AstDoStatement* stmt) {
    if (!stmt) return nullptr;
    return {
        {"kind", "DoStatement"},
        {"condition", serializeExpression(stmt->expression)},
        {"body", serializeStatement(stmt->statement)},
        {"location", tokenLocation(stmt->do_token)}
    };
}

nlohmann::json AstSerializer::serializeForStatement(AstForStatement* stmt) {
    if (!stmt) return nullptr;

    nlohmann::json init = nlohmann::json::array();
    for (unsigned i = 0; i < stmt->NumForInitStatements(); ++i) {
        init.push_back(serializeStatement(stmt->ForInitStatement(i)));
    }

    nlohmann::json update = nlohmann::json::array();
    for (unsigned i = 0; i < stmt->NumForUpdateStatements(); ++i) {
        update.push_back(serializeExpressionStatement(stmt->ForUpdateStatement(i)));
    }

    nlohmann::json result = {
        {"kind", "ForStatement"},
        {"init", init},
        {"update", update},
        {"body", serializeStatement(stmt->statement)},
        {"location", tokenLocation(stmt->for_token)}
    };

    if (stmt->end_expression_opt) {
        result["condition"] = serializeExpression(stmt->end_expression_opt);
    }

    return result;
}

nlohmann::json AstSerializer::serializeForeachStatement(AstForeachStatement* stmt) {
    if (!stmt) return nullptr;
    return {
        {"kind", "ForeachStatement"},
        {"parameter", serializeFormalParameter(stmt->formal_parameter)},
        {"expression", serializeExpression(stmt->expression)},
        {"body", serializeStatement(stmt->statement)},
        {"location", tokenLocation(stmt->for_token)}
    };
}

nlohmann::json AstSerializer::serializeBreakStatement(AstBreakStatement* stmt) {
    if (!stmt) return nullptr;

    nlohmann::json result = {
        {"kind", "BreakStatement"},
        {"location", tokenLocation(stmt->break_token)}
    };

    if (stmt->identifier_token_opt) {
        result["label"] = tokenText(stmt->identifier_token_opt);
    }

    return result;
}

nlohmann::json AstSerializer::serializeContinueStatement(AstContinueStatement* stmt) {
    if (!stmt) return nullptr;

    nlohmann::json result = {
        {"kind", "ContinueStatement"},
        {"location", tokenLocation(stmt->continue_token)}
    };

    if (stmt->identifier_token_opt) {
        result["label"] = tokenText(stmt->identifier_token_opt);
    }

    return result;
}

nlohmann::json AstSerializer::serializeReturnStatement(AstReturnStatement* stmt) {
    if (!stmt) return nullptr;

    nlohmann::json result = {
        {"kind", "ReturnStatement"},
        {"location", tokenLocation(stmt->return_token)}
    };

    if (stmt->expression_opt) {
        result["expression"] = serializeExpression(stmt->expression_opt);
    }

    return result;
}

nlohmann::json AstSerializer::serializeThrowStatement(AstThrowStatement* stmt) {
    if (!stmt) return nullptr;
    return {
        {"kind", "ThrowStatement"},
        {"expression", serializeExpression(stmt->expression)},
        {"location", tokenLocation(stmt->throw_token)}
    };
}

nlohmann::json AstSerializer::serializeSynchronizedStatement(AstSynchronizedStatement* stmt) {
    if (!stmt) return nullptr;
    return {
        {"kind", "SynchronizedStatement"},
        {"expression", serializeExpression(stmt->expression)},
        {"block", serializeBlock(stmt->block)},
        {"location", tokenLocation(stmt->synchronized_token)}
    };
}

nlohmann::json AstSerializer::serializeAssertStatement(AstAssertStatement* stmt) {
    if (!stmt) return nullptr;

    nlohmann::json result = {
        {"kind", "AssertStatement"},
        {"condition", serializeExpression(stmt->condition)},
        {"location", tokenLocation(stmt->assert_token)}
    };

    if (stmt->message_opt) {
        result["message"] = serializeExpression(stmt->message_opt);
    }

    return result;
}

nlohmann::json AstSerializer::serializeCatchClause(AstCatchClause* clause) {
    if (!clause) return nullptr;
    return {
        {"kind", "CatchClause"},
        {"parameter", serializeFormalParameter(clause->formal_parameter)},
        {"block", serializeBlock(clause->block)},
        {"location", tokenLocation(clause->catch_token)}
    };
}

nlohmann::json AstSerializer::serializeFinallyClause(AstFinallyClause* clause) {
    if (!clause) return nullptr;
    return {
        {"kind", "FinallyClause"},
        {"block", serializeBlock(clause->block)},
        {"location", tokenLocation(clause->finally_token)}
    };
}

nlohmann::json AstSerializer::serializeTryStatement(AstTryStatement* stmt) {
    if (!stmt) return nullptr;

    nlohmann::json catches = nlohmann::json::array();
    for (unsigned i = 0; i < stmt->NumCatchClauses(); ++i) {
        catches.push_back(serializeCatchClause(stmt->CatchClause(i)));
    }

    nlohmann::json result = {
        {"kind", "TryStatement"},
        {"block", serializeBlock(stmt->block)},
        {"catch_clauses", catches},
        {"location", tokenLocation(stmt->try_token)}
    };

    if (stmt->finally_clause_opt) {
        result["finally_clause"] = serializeFinallyClause(stmt->finally_clause_opt);
    }

    return result;
}

nlohmann::json AstSerializer::serializeIntegerLiteral(AstIntegerLiteral* lit) {
    if (!lit) return nullptr;
    return {
        {"kind", "IntegerLiteral"},
        {"value", tokenText(lit->integer_literal_token)},
        {"location", tokenLocation(lit->integer_literal_token)}
    };
}

nlohmann::json AstSerializer::serializeLongLiteral(AstLongLiteral* lit) {
    if (!lit) return nullptr;
    return {
        {"kind", "LongLiteral"},
        {"value", tokenText(lit->long_literal_token)},
        {"location", tokenLocation(lit->long_literal_token)}
    };
}

nlohmann::json AstSerializer::serializeFloatLiteral(AstFloatLiteral* lit) {
    if (!lit) return nullptr;
    return {
        {"kind", "FloatLiteral"},
        {"value", tokenText(lit->float_literal_token)},
        {"location", tokenLocation(lit->float_literal_token)}
    };
}

nlohmann::json AstSerializer::serializeDoubleLiteral(AstDoubleLiteral* lit) {
    if (!lit) return nullptr;
    return {
        {"kind", "DoubleLiteral"},
        {"value", tokenText(lit->double_literal_token)},
        {"location", tokenLocation(lit->double_literal_token)}
    };
}

nlohmann::json AstSerializer::serializeTrueLiteral(AstTrueLiteral* lit) {
    if (!lit) return nullptr;
    return {
        {"kind", "TrueLiteral"},
        {"location", tokenLocation(lit->true_literal_token)}
    };
}

nlohmann::json AstSerializer::serializeFalseLiteral(AstFalseLiteral* lit) {
    if (!lit) return nullptr;
    return {
        {"kind", "FalseLiteral"},
        {"location", tokenLocation(lit->false_literal_token)}
    };
}

nlohmann::json AstSerializer::serializeStringLiteral(AstStringLiteral* lit) {
    if (!lit) return nullptr;
    return {
        {"kind", "StringLiteral"},
        {"value", tokenText(lit->string_literal_token)},
        {"location", tokenLocation(lit->string_literal_token)}
    };
}

nlohmann::json AstSerializer::serializeCharacterLiteral(AstCharacterLiteral* lit) {
    if (!lit) return nullptr;
    return {
        {"kind", "CharacterLiteral"},
        {"value", tokenText(lit->character_literal_token)},
        {"location", tokenLocation(lit->character_literal_token)}
    };
}

nlohmann::json AstSerializer::serializeNullLiteral(AstNullLiteral* lit) {
    if (!lit) return nullptr;
    return {
        {"kind", "NullLiteral"},
        {"location", tokenLocation(lit->null_token)}
    };
}

nlohmann::json AstSerializer::serializeClassLiteral(AstClassLiteral* lit) {
    if (!lit) return nullptr;
    return {
        {"kind", "ClassLiteral"},
        {"type", serializeType(lit->type)},
        {"location", tokenLocation(lit->LeftToken())}
    };
}

nlohmann::json AstSerializer::serializeThisExpression(AstThisExpression* expr) {
    if (!expr) return nullptr;

    nlohmann::json result = {
        {"kind", "ThisExpression"},
        {"location", tokenLocation(expr->this_token)}
    };

    if (expr->base_opt) {
        result["base"] = serializeTypeName(expr->base_opt);
    }

    return result;
}

nlohmann::json AstSerializer::serializeSuperExpression(AstSuperExpression* expr) {
    if (!expr) return nullptr;

    nlohmann::json result = {
        {"kind", "SuperExpression"},
        {"location", tokenLocation(expr->super_token)}
    };

    if (expr->base_opt) {
        result["base"] = serializeTypeName(expr->base_opt);
    }

    return result;
}

nlohmann::json AstSerializer::serializeParenthesizedExpression(AstParenthesizedExpression* expr) {
    if (!expr) return nullptr;
    return {
        {"kind", "ParenthesizedExpression"},
        {"expression", serializeExpression(expr->expression)},
        {"location", tokenLocation(expr->left_parenthesis_token)}
    };
}

nlohmann::json AstSerializer::serializeClassCreationExpression(AstClassCreationExpression* expr) {
    if (!expr) return nullptr;

    nlohmann::json result = {
        {"kind", "ClassCreationExpression"},
        {"type", serializeTypeName(expr->class_type)},
        {"arguments", serializeArguments(expr->arguments)},
        {"location", tokenLocation(expr->new_token)}
    };

    if (expr->base_opt) {
        result["base"] = serializeExpression(expr->base_opt);
    }
    if (expr->type_arguments_opt) {
        result["type_arguments"] = serializeTypeArguments(expr->type_arguments_opt);
    }
    if (expr->class_body_opt) {
        result["body"] = serializeClassBody(expr->class_body_opt);
    }

    return result;
}

nlohmann::json AstSerializer::serializeDimExpr(AstDimExpr* expr) {
    if (!expr) return nullptr;
    return {
        {"kind", "DimExpr"},
        {"expression", serializeExpression(expr->expression)},
        {"location", tokenLocation(expr->left_bracket_token)}
    };
}

nlohmann::json AstSerializer::serializeArrayCreationExpression(AstArrayCreationExpression* expr) {
    if (!expr) return nullptr;

    nlohmann::json dimExprs = nlohmann::json::array();
    for (unsigned i = 0; i < expr->NumDimExprs(); ++i) {
        dimExprs.push_back(serializeDimExpr(expr->DimExpr(i)));
    }

    nlohmann::json result = {
        {"kind", "ArrayCreationExpression"},
        {"type", serializeType(expr->array_type)},
        {"dim_exprs", dimExprs},
        {"location", tokenLocation(expr->new_token)}
    };

    if (expr->brackets_opt) {
        result["brackets"] = serializeBrackets(expr->brackets_opt);
    }
    if (expr->array_initializer_opt) {
        result["initializer"] = serializeArrayInitializer(expr->array_initializer_opt);
    }

    return result;
}

nlohmann::json AstSerializer::serializeFieldAccess(AstFieldAccess* expr) {
    if (!expr) return nullptr;
    return {
        {"kind", "FieldAccess"},
        {"base", serializeExpression(expr->base)},
        {"identifier", tokenText(expr->identifier_token)},
        {"location", tokenLocation(expr->LeftToken())}
    };
}

nlohmann::json AstSerializer::serializeMethodInvocation(AstMethodInvocation* expr) {
    if (!expr) return nullptr;

    nlohmann::json result = {
        {"kind", "MethodInvocation"},
        {"arguments", serializeArguments(expr->arguments)},
        {"location", tokenLocation(expr->LeftToken())}
    };

    if (expr->base_opt) {
        result["base"] = serializeExpression(expr->base_opt);
    }
    if (expr->type_arguments_opt) {
        result["type_arguments"] = serializeTypeArguments(expr->type_arguments_opt);
    }
    result["identifier"] = tokenText(expr->identifier_token);

    return result;
}

nlohmann::json AstSerializer::serializeArrayAccess(AstArrayAccess* expr) {
    if (!expr) return nullptr;
    return {
        {"kind", "ArrayAccess"},
        {"base", serializeExpression(expr->base)},
        {"index", serializeExpression(expr->expression)},
        {"location", tokenLocation(expr->LeftToken())}
    };
}

nlohmann::json AstSerializer::serializePostUnaryExpression(AstPostUnaryExpression* expr) {
    if (!expr) return nullptr;

    std::string op;
    switch (expr->Tag()) {
        case AstPostUnaryExpression::PLUSPLUS: op = "++"; break;
        case AstPostUnaryExpression::MINUSMINUS: op = "--"; break;
        default: op = "unknown"; break;
    }

    return {
        {"kind", "PostUnaryExpression"},
        {"operator", op},
        {"expression", serializeExpression(expr->expression)},
        {"location", tokenLocation(expr->LeftToken())}
    };
}

nlohmann::json AstSerializer::serializePreUnaryExpression(AstPreUnaryExpression* expr) {
    if (!expr) return nullptr;

    std::string op;
    switch (expr->Tag()) {
        case AstPreUnaryExpression::PLUSPLUS: op = "++"; break;
        case AstPreUnaryExpression::MINUSMINUS: op = "--"; break;
        case AstPreUnaryExpression::PLUS: op = "+"; break;
        case AstPreUnaryExpression::MINUS: op = "-"; break;
        case AstPreUnaryExpression::TWIDDLE: op = "~"; break;
        case AstPreUnaryExpression::NOT: op = "!"; break;
        default: op = "unknown"; break;
    }

    return {
        {"kind", "PreUnaryExpression"},
        {"operator", op},
        {"expression", serializeExpression(expr->expression)},
        {"location", tokenLocation(expr->LeftToken())}
    };
}

nlohmann::json AstSerializer::serializeCastExpression(AstCastExpression* expr) {
    if (!expr) return nullptr;
    return {
        {"kind", "CastExpression"},
        {"type", serializeType(expr->type)},
        {"expression", serializeExpression(expr->expression)},
        {"location", tokenLocation(expr->left_parenthesis_token)}
    };
}

nlohmann::json AstSerializer::serializeBinaryExpression(AstBinaryExpression* expr) {
    if (!expr) return nullptr;

    std::string op;
    switch (expr->Tag()) {
        case AstBinaryExpression::STAR: op = "*"; break;
        case AstBinaryExpression::SLASH: op = "/"; break;
        case AstBinaryExpression::MOD: op = "%"; break;
        case AstBinaryExpression::PLUS: op = "+"; break;
        case AstBinaryExpression::MINUS: op = "-"; break;
        case AstBinaryExpression::LEFT_SHIFT: op = "<<"; break;
        case AstBinaryExpression::RIGHT_SHIFT: op = ">>"; break;
        case AstBinaryExpression::UNSIGNED_RIGHT_SHIFT: op = ">>>"; break;
        case AstBinaryExpression::LESS: op = "<"; break;
        case AstBinaryExpression::GREATER: op = ">"; break;
        case AstBinaryExpression::LESS_EQUAL: op = "<="; break;
        case AstBinaryExpression::GREATER_EQUAL: op = ">="; break;
        case AstBinaryExpression::EQUAL_EQUAL: op = "=="; break;
        case AstBinaryExpression::NOT_EQUAL: op = "!="; break;
        case AstBinaryExpression::AND: op = "&"; break;
        case AstBinaryExpression::XOR: op = "^"; break;
        case AstBinaryExpression::IOR: op = "|"; break;
        case AstBinaryExpression::AND_AND: op = "&&"; break;
        case AstBinaryExpression::OR_OR: op = "||"; break;
        default: op = "unknown"; break;
    }

    return {
        {"kind", "BinaryExpression"},
        {"operator", op},
        {"left", serializeExpression(expr->left_expression)},
        {"right", serializeExpression(expr->right_expression)},
        {"location", tokenLocation(expr->LeftToken())}
    };
}

nlohmann::json AstSerializer::serializeInstanceofExpression(AstInstanceofExpression* expr) {
    if (!expr) return nullptr;
    return {
        {"kind", "InstanceofExpression"},
        {"expression", serializeExpression(expr->expression)},
        {"type", serializeType(expr->type)},
        {"location", tokenLocation(expr->LeftToken())}
    };
}

nlohmann::json AstSerializer::serializeConditionalExpression(AstConditionalExpression* expr) {
    if (!expr) return nullptr;
    return {
        {"kind", "ConditionalExpression"},
        {"condition", serializeExpression(expr->test_expression)},
        {"true_expression", serializeExpression(expr->true_expression)},
        {"false_expression", serializeExpression(expr->false_expression)},
        {"location", tokenLocation(expr->LeftToken())}
    };
}

nlohmann::json AstSerializer::serializeAssignmentExpression(AstAssignmentExpression* expr) {
    if (!expr) return nullptr;

    std::string op;
    switch (expr->Tag()) {
        case AstAssignmentExpression::SIMPLE_EQUAL: op = "="; break;
        case AstAssignmentExpression::STAR_EQUAL: op = "*="; break;
        case AstAssignmentExpression::SLASH_EQUAL: op = "/="; break;
        case AstAssignmentExpression::MOD_EQUAL: op = "%="; break;
        case AstAssignmentExpression::PLUS_EQUAL: op = "+="; break;
        case AstAssignmentExpression::MINUS_EQUAL: op = "-="; break;
        case AstAssignmentExpression::LEFT_SHIFT_EQUAL: op = "<<="; break;
        case AstAssignmentExpression::RIGHT_SHIFT_EQUAL: op = ">>="; break;
        case AstAssignmentExpression::UNSIGNED_RIGHT_SHIFT_EQUAL: op = ">>>="; break;
        case AstAssignmentExpression::AND_EQUAL: op = "&="; break;
        case AstAssignmentExpression::XOR_EQUAL: op = "^="; break;
        case AstAssignmentExpression::IOR_EQUAL: op = "|="; break;
        default: op = "unknown"; break;
    }

    return {
        {"kind", "AssignmentExpression"},
        {"operator", op},
        {"left", serializeExpression(expr->left_hand_side)},
        {"right", serializeExpression(expr->expression)},
        {"location", tokenLocation(expr->LeftToken())}
    };
}

} // namespace Jopa
