#include "ast_json.h"
#include "ast.h"
#include "stream.h"
#include <cstring>
#include <cstdint>

namespace Jopa {

// Helper to check if a pointer appears valid (not null or suspiciously small)
static inline bool IsValidPointer(const void* ptr)
{
    // Pointers less than 0x1000 are almost certainly invalid in userspace
    return ptr && reinterpret_cast<uintptr_t>(ptr) >= 0x1000;
}

// Convert AstKind enum to string
const char* AstKindName(int kind)
{
    static const char* names[] = {
        "AST",
        // Expressions
        "Name", "Dot", "IntegerLiteral", "LongLiteral", "FloatLiteral",
        "DoubleLiteral", "TrueLiteral", "FalseLiteral", "StringLiteral",
        "CharacterLiteral", "NullLiteral", "ClassLiteral", "ThisExpression",
        "SuperExpression", "ParenthesizedExpression", "ArrayAccess", "Call",
        "ClassCreation", "ArrayCreation", "PostUnary", "PreUnary", "Cast",
        "Binary", "Instanceof", "Conditional", "Assignment",
        "_num_expression_kinds",
        // Statements
        "ThisCall", "SuperCall", "Block", "If", "EmptyStatement",
        "ExpressionStatement", "Switch", "SwitchBlock",
        "LocalVariableDeclaration", "LocalClass", "While", "Do", "For",
        "Foreach", "Break", "Continue", "Return", "Throw",
        "SynchronizedStatement", "Assert", "Try",
        "_num_expr_or_stmt_kinds",
        // All others
        "Arguments", "Dim", "ListNode",
        "Int", "Double", "Char", "Long", "Float", "Byte", "Short", "Boolean", "Void",
        "Array", "Wildcard", "TypeArguments", "Type", "Compilation",
        "MemberValuePair", "Annotation", "ModifierKeyword", "Modifiers",
        "Package", "Import", "EmptyDeclaration", "Class", "TypeParam",
        "ParamList", "ClassBody", "Field", "VariableDeclarator",
        "VariableDeclaratorName", "Brackets", "Method", "MethodDeclarator",
        "Parameter", "Constructor", "EnumType", "Enum", "Interface",
        "AnnotationType", "ArrayInitializer", "Initializer", "MethodBody",
        "SwitchLabel", "Catch", "Finally"
    };

    if (kind >= 0 && kind < static_cast<int>(sizeof(names) / sizeof(names[0])))
        return names[kind];
    return "Unknown";
}

const char* AstJsonSerializer::KindToString(int kind)
{
    return AstKindName(kind);
}

AstJsonSerializer::AstJsonSerializer(LexStream* lex_stream)
    : lex_stream_(lex_stream)
{
}

json AstJsonSerializer::TokenLocation(unsigned token_index)
{
    if (!lex_stream_ || token_index == 0 || token_index >= lex_stream_->NumTokens())
        return nullptr;

    return json{
        {"line", lex_stream_->Line(token_index)},
        {"column", lex_stream_->Column(token_index)}
    };
}

std::string AstJsonSerializer::TokenText(unsigned token_index)
{
    if (!lex_stream_ || token_index == 0 || token_index >= lex_stream_->NumTokens())
        return "";

    const wchar_t* name = lex_stream_->NameString(token_index);
    if (!name)
        return "";

    // Convert wide string to UTF-8, handling UTF-16 surrogate pairs
    std::string result;
    for (const wchar_t* p = name; *p; ++p)
    {
        uint32_t ch = static_cast<uint32_t>(*p);

        // Handle UTF-16 surrogate pairs
        if (ch >= 0xD800 && ch <= 0xDBFF)
        {
            // High surrogate - check for low surrogate
            if (p[1] >= 0xDC00 && p[1] <= 0xDFFF)
            {
                // Combine surrogate pair into a single code point
                uint32_t low = static_cast<uint32_t>(p[1]);
                ch = ((ch - 0xD800) << 10) + (low - 0xDC00) + 0x10000;
                ++p; // Skip the low surrogate
            }
            else
            {
                // Lone high surrogate - use replacement character
                ch = 0xFFFD;
            }
        }
        else if (ch >= 0xDC00 && ch <= 0xDFFF)
        {
            // Lone low surrogate - use replacement character
            ch = 0xFFFD;
        }

        // Encode as UTF-8
        if (ch < 0x80)
        {
            result += static_cast<char>(ch);
        }
        else if (ch < 0x800)
        {
            result += static_cast<char>(0xC0 | (ch >> 6));
            result += static_cast<char>(0x80 | (ch & 0x3F));
        }
        else if (ch < 0x10000)
        {
            result += static_cast<char>(0xE0 | (ch >> 12));
            result += static_cast<char>(0x80 | ((ch >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (ch & 0x3F));
        }
        else
        {
            // Supplementary plane characters (U+10000 to U+10FFFF) - 4 bytes
            result += static_cast<char>(0xF0 | (ch >> 18));
            result += static_cast<char>(0x80 | ((ch >> 12) & 0x3F));
            result += static_cast<char>(0x80 | ((ch >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (ch & 0x3F));
        }
    }
    return result;
}

json AstJsonSerializer::Serialize(AstCompilationUnit* compilation_unit)
{
    if (!compilation_unit)
        return nullptr;
    return SerializeCompilationUnit(compilation_unit);
}

json AstJsonSerializer::SerializeNode(Ast* node)
{
    if (!IsValidPointer(node))
        return nullptr;

    switch (node->kind)
    {
    // Compilation unit
    case Ast::COMPILATION:
        return SerializeCompilationUnit(static_cast<AstCompilationUnit*>(node));

    // Package and imports
    case Ast::PACKAGE:
        return SerializePackageDeclaration(static_cast<AstPackageDeclaration*>(node));
    case Ast::IMPORT:
        return SerializeImportDeclaration(static_cast<AstImportDeclaration*>(node));

    // Type declarations
    case Ast::CLASS:
        return SerializeClassDeclaration(static_cast<AstClassDeclaration*>(node));
    case Ast::INTERFACE:
        return SerializeInterfaceDeclaration(static_cast<AstInterfaceDeclaration*>(node));
    case Ast::ENUM_TYPE:
        return SerializeEnumDeclaration(static_cast<AstEnumDeclaration*>(node));
    case Ast::ANNOTATION_TYPE:
        return SerializeAnnotationDeclaration(static_cast<AstAnnotationDeclaration*>(node));
    case Ast::EMPTY_DECLARATION:
        return SerializeEmptyDeclaration(static_cast<AstEmptyDeclaration*>(node));
    case Ast::CLASS_BODY:
        return SerializeClassBody(static_cast<AstClassBody*>(node));
    case Ast::ENUM:
        return SerializeEnumConstant(static_cast<AstEnumConstant*>(node));

    // Type references
    case Ast::INT:
    case Ast::DOUBLE:
    case Ast::CHAR:
    case Ast::LONG:
    case Ast::FLOAT:
    case Ast::BYTE:
    case Ast::SHORT:
    case Ast::BOOLEAN:
    case Ast::VOID_TYPE:
        return SerializePrimitiveType(static_cast<AstPrimitiveType*>(node));
    case Ast::TYPE:
        return SerializeTypeName(static_cast<AstTypeName*>(node));
    case Ast::ARRAY:
        return SerializeArrayType(static_cast<AstArrayType*>(node));
    case Ast::WILDCARD:
        return SerializeWildcard(static_cast<AstWildcard*>(node));
    case Ast::TYPE_ARGUMENTS:
        return SerializeTypeArguments(static_cast<AstTypeArguments*>(node));
    case Ast::TYPE_PARAM:
        return SerializeTypeParameter(static_cast<AstTypeParameter*>(node));
    case Ast::PARAM_LIST:
        return SerializeTypeParameters(static_cast<AstTypeParameters*>(node));

    // Members
    case Ast::FIELD:
        return SerializeFieldDeclaration(static_cast<AstFieldDeclaration*>(node));
    case Ast::METHOD:
        return SerializeMethodDeclaration(static_cast<AstMethodDeclaration*>(node));
    case Ast::CONSTRUCTOR:
        return SerializeConstructorDeclaration(static_cast<AstConstructorDeclaration*>(node));
    case Ast::INITIALIZER:
        return SerializeInitializerDeclaration(static_cast<AstInitializerDeclaration*>(node));
    case Ast::PARAMETER:
        return SerializeFormalParameter(static_cast<AstFormalParameter*>(node));
    case Ast::VARIABLE_DECLARATOR:
        return SerializeVariableDeclarator(static_cast<AstVariableDeclarator*>(node));
    case Ast::VARIABLE_DECLARATOR_NAME:
        return SerializeVariableDeclaratorId(static_cast<AstVariableDeclaratorId*>(node));
    case Ast::METHOD_DECLARATOR:
        return SerializeMethodDeclarator(static_cast<AstMethodDeclarator*>(node));
    case Ast::METHOD_BODY:
        return SerializeMethodBody(static_cast<AstMethodBody*>(node));

    // Modifiers and annotations
    case Ast::MODIFIERS:
        return SerializeModifiers(static_cast<AstModifiers*>(node));
    case Ast::ANNOTATION:
        return SerializeAnnotation(static_cast<AstAnnotation*>(node));
    case Ast::MEMBER_VALUE_PAIR:
        return SerializeMemberValuePair(static_cast<AstMemberValuePair*>(node));

    // Statements
    case Ast::BLOCK:
        return SerializeBlock(static_cast<AstBlock*>(node));
    case Ast::LOCAL_VARIABLE_DECLARATION:
        return SerializeLocalVariableStatement(static_cast<AstLocalVariableStatement*>(node));
    case Ast::LOCAL_CLASS:
        return SerializeLocalClassStatement(static_cast<AstLocalClassStatement*>(node));
    case Ast::IF:
        return SerializeIfStatement(static_cast<AstIfStatement*>(node));
    case Ast::EMPTY_STATEMENT:
        return SerializeEmptyStatement(static_cast<AstEmptyStatement*>(node));
    case Ast::EXPRESSION_STATEMENT:
        return SerializeExpressionStatement(static_cast<AstExpressionStatement*>(node));
    case Ast::SWITCH:
        return SerializeSwitchStatement(static_cast<AstSwitchStatement*>(node));
    case Ast::SWITCH_BLOCK:
        return SerializeSwitchBlockStatement(static_cast<AstSwitchBlockStatement*>(node));
    case Ast::SWITCH_LABEL:
        return SerializeSwitchLabel(static_cast<AstSwitchLabel*>(node));
    case Ast::WHILE:
        return SerializeWhileStatement(static_cast<AstWhileStatement*>(node));
    case Ast::DO:
        return SerializeDoStatement(static_cast<AstDoStatement*>(node));
    case Ast::FOR:
        return SerializeForStatement(static_cast<AstForStatement*>(node));
    case Ast::FOREACH:
        return SerializeForeachStatement(static_cast<AstForeachStatement*>(node));
    case Ast::BREAK:
        return SerializeBreakStatement(static_cast<AstBreakStatement*>(node));
    case Ast::CONTINUE:
        return SerializeContinueStatement(static_cast<AstContinueStatement*>(node));
    case Ast::RETURN:
        return SerializeReturnStatement(static_cast<AstReturnStatement*>(node));
    case Ast::THROW:
        return SerializeThrowStatement(static_cast<AstThrowStatement*>(node));
    case Ast::SYNCHRONIZED_STATEMENT:
        return SerializeSynchronizedStatement(static_cast<AstSynchronizedStatement*>(node));
    case Ast::ASSERT:
        return SerializeAssertStatement(static_cast<AstAssertStatement*>(node));
    case Ast::TRY:
        return SerializeTryStatement(static_cast<AstTryStatement*>(node));
    case Ast::CATCH:
        return SerializeCatchClause(static_cast<AstCatchClause*>(node));
    case Ast::FINALLY:
        return SerializeFinallyClause(static_cast<AstFinallyClause*>(node));

    // Expressions
    case Ast::NAME:
        return SerializeName(static_cast<AstName*>(node));
    case Ast::DOT:
        return SerializeFieldAccess(static_cast<AstFieldAccess*>(node));
    case Ast::INTEGER_LITERAL:
        return SerializeIntegerLiteral(static_cast<AstIntegerLiteral*>(node));
    case Ast::LONG_LITERAL:
        return SerializeLongLiteral(static_cast<AstLongLiteral*>(node));
    case Ast::FLOAT_LITERAL:
        return SerializeFloatLiteral(static_cast<AstFloatLiteral*>(node));
    case Ast::DOUBLE_LITERAL:
        return SerializeDoubleLiteral(static_cast<AstDoubleLiteral*>(node));
    case Ast::TRUE_LITERAL:
        return SerializeTrueLiteral(static_cast<AstTrueLiteral*>(node));
    case Ast::FALSE_LITERAL:
        return SerializeFalseLiteral(static_cast<AstFalseLiteral*>(node));
    case Ast::STRING_LITERAL:
        return SerializeStringLiteral(static_cast<AstStringLiteral*>(node));
    case Ast::CHARACTER_LITERAL:
        return SerializeCharacterLiteral(static_cast<AstCharacterLiteral*>(node));
    case Ast::NULL_LITERAL:
        return SerializeNullLiteral(static_cast<AstNullLiteral*>(node));
    case Ast::CLASS_LITERAL:
        return SerializeClassLiteral(static_cast<AstClassLiteral*>(node));
    case Ast::THIS_EXPRESSION:
        return SerializeThisExpression(static_cast<AstThisExpression*>(node));
    case Ast::SUPER_EXPRESSION:
        return SerializeSuperExpression(static_cast<AstSuperExpression*>(node));
    case Ast::PARENTHESIZED_EXPRESSION:
        return SerializeParenthesizedExpression(static_cast<AstParenthesizedExpression*>(node));
    case Ast::ARRAY_ACCESS:
        return SerializeArrayAccess(static_cast<AstArrayAccess*>(node));
    case Ast::CALL:
        return SerializeMethodInvocation(static_cast<AstMethodInvocation*>(node));
    case Ast::CLASS_CREATION:
        return SerializeClassCreationExpression(static_cast<AstClassCreationExpression*>(node));
    case Ast::ARRAY_CREATION:
        return SerializeArrayCreationExpression(static_cast<AstArrayCreationExpression*>(node));
    case Ast::DIM:
        return SerializeDimExpr(static_cast<AstDimExpr*>(node));
    case Ast::POST_UNARY:
        return SerializePostUnaryExpression(static_cast<AstPostUnaryExpression*>(node));
    case Ast::PRE_UNARY:
        return SerializePreUnaryExpression(static_cast<AstPreUnaryExpression*>(node));
    case Ast::CAST:
        return SerializeCastExpression(static_cast<AstCastExpression*>(node));
    case Ast::BINARY:
        return SerializeBinaryExpression(static_cast<AstBinaryExpression*>(node));
    case Ast::INSTANCEOF:
        return SerializeInstanceofExpression(static_cast<AstInstanceofExpression*>(node));
    case Ast::CONDITIONAL:
        return SerializeConditionalExpression(static_cast<AstConditionalExpression*>(node));
    case Ast::ASSIGNMENT:
        return SerializeAssignmentExpression(static_cast<AstAssignmentExpression*>(node));
    case Ast::ARRAY_INITIALIZER:
        return SerializeArrayInitializer(static_cast<AstArrayInitializer*>(node));
    case Ast::THIS_CALL:
        return SerializeThisCall(static_cast<AstThisCall*>(node));
    case Ast::SUPER_CALL:
        return SerializeSuperCall(static_cast<AstSuperCall*>(node));
    case Ast::ARGUMENTS:
        return SerializeArguments(static_cast<AstArguments*>(node));
    case Ast::BRACKETS:
        return SerializeBrackets(static_cast<AstBrackets*>(node));

    default:
        return json{{"kind", KindToString(node->kind)}, {"error", "Unknown node type"}};
    }
}

// Compilation unit
json AstJsonSerializer::SerializeCompilationUnit(AstCompilationUnit* node)
{
    json result = {
        {"kind", "CompilationUnit"},
        {"file", lex_stream_ ? lex_stream_->FileName() : nullptr}
    };

    if (node->package_declaration_opt)
        result["package"] = SerializePackageDeclaration(node->package_declaration_opt);

    json imports = json::array();
    for (unsigned i = 0; i < node->NumImportDeclarations(); i++)
        imports.push_back(SerializeImportDeclaration(node->ImportDeclaration(i)));
    result["imports"] = imports;

    json types = json::array();
    for (unsigned i = 0; i < node->NumTypeDeclarations(); i++)
        types.push_back(SerializeNode(node->TypeDeclaration(i)));
    result["types"] = types;

    return result;
}

json AstJsonSerializer::SerializePackageDeclaration(AstPackageDeclaration* node)
{
    return json{
        {"kind", "PackageDeclaration"},
        {"name", SerializeNode(node->name)},
        {"location", TokenLocation(node->package_token)}
    };
}

json AstJsonSerializer::SerializeImportDeclaration(AstImportDeclaration* node)
{
    return json{
        {"kind", "ImportDeclaration"},
        {"name", SerializeNode(node->name)},
        {"isStatic", node->static_token_opt != 0},
        {"isOnDemand", node->star_token_opt != 0},
        {"location", TokenLocation(node->import_token)}
    };
}

// Type declarations
json AstJsonSerializer::SerializeClassDeclaration(AstClassDeclaration* node)
{
    json result = {
        {"kind", "ClassDeclaration"},
        {"name", TokenText(node->class_body->identifier_token)},
        {"location", TokenLocation(node->class_body->identifier_token)}
    };

    if (node->modifiers_opt)
        result["modifiers"] = SerializeModifiers(node->modifiers_opt);

    if (node->type_parameters_opt)
        result["typeParameters"] = SerializeTypeParameters(node->type_parameters_opt);

    if (node->super_opt)
        result["extends"] = SerializeNode(node->super_opt);

    json interfaces = json::array();
    for (unsigned i = 0; i < node->NumInterfaces(); i++)
        interfaces.push_back(SerializeNode(node->Interface(i)));
    if (!interfaces.empty())
        result["implements"] = interfaces;

    result["body"] = SerializeClassBody(node->class_body);

    return result;
}

json AstJsonSerializer::SerializeInterfaceDeclaration(AstInterfaceDeclaration* node)
{
    json result = {
        {"kind", "InterfaceDeclaration"},
        {"name", TokenText(node->class_body->identifier_token)},
        {"location", TokenLocation(node->class_body->identifier_token)}
    };

    if (node->modifiers_opt)
        result["modifiers"] = SerializeModifiers(node->modifiers_opt);

    if (node->type_parameters_opt)
        result["typeParameters"] = SerializeTypeParameters(node->type_parameters_opt);

    json interfaces = json::array();
    for (unsigned i = 0; i < node->NumInterfaces(); i++)
        interfaces.push_back(SerializeNode(node->Interface(i)));
    if (!interfaces.empty())
        result["extends"] = interfaces;

    result["body"] = SerializeClassBody(node->class_body);

    return result;
}

json AstJsonSerializer::SerializeEnumDeclaration(AstEnumDeclaration* node)
{
    json result = {
        {"kind", "EnumDeclaration"},
        {"name", TokenText(node->class_body->identifier_token)},
        {"location", TokenLocation(node->class_body->identifier_token)}
    };

    if (node->modifiers_opt)
        result["modifiers"] = SerializeModifiers(node->modifiers_opt);

    json interfaces = json::array();
    for (unsigned i = 0; i < node->NumInterfaces(); i++)
        interfaces.push_back(SerializeNode(node->Interface(i)));
    if (!interfaces.empty())
        result["implements"] = interfaces;

    json constants = json::array();
    for (unsigned i = 0; i < node->NumEnumConstants(); i++)
        constants.push_back(SerializeEnumConstant(node->EnumConstant(i)));
    result["constants"] = constants;

    result["body"] = SerializeClassBody(node->class_body);

    return result;
}

json AstJsonSerializer::SerializeAnnotationDeclaration(AstAnnotationDeclaration* node)
{
    json result = {
        {"kind", "AnnotationDeclaration"},
        {"name", TokenText(node->class_body->identifier_token)},
        {"location", TokenLocation(node->class_body->identifier_token)}
    };

    if (node->modifiers_opt)
        result["modifiers"] = SerializeModifiers(node->modifiers_opt);

    result["body"] = SerializeClassBody(node->class_body);

    return result;
}

json AstJsonSerializer::SerializeEmptyDeclaration(AstEmptyDeclaration* node)
{
    return json{
        {"kind", "EmptyDeclaration"},
        {"location", TokenLocation(node->semicolon_token)}
    };
}

json AstJsonSerializer::SerializeClassBody(AstClassBody* node)
{
    json members = json::array();
    for (unsigned i = 0; i < node->NumClassBodyDeclarations(); i++)
        members.push_back(SerializeNode(node->ClassBodyDeclaration(i)));

    return json{
        {"kind", "ClassBody"},
        {"members", members}
    };
}

json AstJsonSerializer::SerializeEnumConstant(AstEnumConstant* node)
{
    json result = {
        {"kind", "EnumConstant"},
        {"name", TokenText(node->identifier_token)},
        {"location", TokenLocation(node->identifier_token)}
    };

    if (node->modifiers_opt)
        result["modifiers"] = SerializeModifiers(node->modifiers_opt);

    if (node->arguments_opt)
        result["arguments"] = SerializeArguments(node->arguments_opt);

    if (node->class_body_opt)
        result["body"] = SerializeClassBody(node->class_body_opt);

    return result;
}

// Type references
json AstJsonSerializer::SerializeType(AstType* node)
{
    if (!node)
        return nullptr;
    return SerializeNode(node);
}

json AstJsonSerializer::SerializePrimitiveType(AstPrimitiveType* node)
{
    const char* typeName = nullptr;
    switch (node->kind)
    {
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

    return json{
        {"kind", "PrimitiveType"},
        {"name", typeName},
        {"location", TokenLocation(node->primitive_kind_token)}
    };
}

json AstJsonSerializer::SerializeTypeName(AstTypeName* node)
{
    if (!IsValidPointer(node))
        return nullptr;

    json result = {
        {"kind", "TypeName"}
    };

    if (IsValidPointer(node->name))
    {
        result["name"] = SerializeNode(node->name);
        result["location"] = TokenLocation(node->name->LeftToken());
    }

    if (IsValidPointer(node->type_arguments_opt))
        result["typeArguments"] = SerializeTypeArguments(node->type_arguments_opt);

    return result;
}

json AstJsonSerializer::SerializeArrayType(AstArrayType* node)
{
    return json{
        {"kind", "ArrayType"},
        {"elementType", SerializeNode(node->type)},
        {"dimensions", node->NumBrackets()},
        {"location", TokenLocation(node->type->LeftToken())}
    };
}

json AstJsonSerializer::SerializeWildcard(AstWildcard* node)
{
    json result = {
        {"kind", "Wildcard"},
        {"location", TokenLocation(node->question_token)}
    };

    if (node->extends_token_opt)
    {
        result["bound"] = SerializeNode(node->bounds_opt);
        result["boundKind"] = "extends";
    }
    else if (node->super_token_opt)
    {
        result["bound"] = SerializeNode(node->bounds_opt);
        result["boundKind"] = "super";
    }

    return result;
}

json AstJsonSerializer::SerializeTypeArguments(AstTypeArguments* node)
{
    json args = json::array();
    for (unsigned i = 0; i < node->NumTypeArguments(); i++)
        args.push_back(SerializeNode(node->TypeArgument(i)));

    return json{
        {"kind", "TypeArguments"},
        {"arguments", args}
    };
}

json AstJsonSerializer::SerializeTypeParameter(AstTypeParameter* node)
{
    json result = {
        {"kind", "TypeParameter"},
        {"name", TokenText(node->identifier_token)},
        {"location", TokenLocation(node->identifier_token)}
    };

    json bounds = json::array();
    for (unsigned i = 0; i < node->NumBounds(); i++)
        bounds.push_back(SerializeNode(node->Bound(i)));
    if (!bounds.empty())
        result["bounds"] = bounds;

    return result;
}

json AstJsonSerializer::SerializeTypeParameters(AstTypeParameters* node)
{
    json params = json::array();
    for (unsigned i = 0; i < node->NumTypeParameters(); i++)
        params.push_back(SerializeTypeParameter(node->TypeParameter(i)));

    return json{
        {"kind", "TypeParameters"},
        {"parameters", params}
    };
}

// Members
json AstJsonSerializer::SerializeFieldDeclaration(AstFieldDeclaration* node)
{
    json declarators = json::array();
    for (unsigned i = 0; i < node->NumVariableDeclarators(); i++)
        declarators.push_back(SerializeVariableDeclarator(node->VariableDeclarator(i)));

    json result = {
        {"kind", "FieldDeclaration"},
        {"type", SerializeNode(node->type)},
        {"declarators", declarators},
        {"location", TokenLocation(node->type->LeftToken())}
    };

    if (node->modifiers_opt)
        result["modifiers"] = SerializeModifiers(node->modifiers_opt);

    return result;
}

json AstJsonSerializer::SerializeMethodDeclaration(AstMethodDeclaration* node)
{
    json result = {
        {"kind", "MethodDeclaration"},
        {"name", TokenText(node->method_declarator->identifier_token)},
        {"returnType", SerializeNode(node->type)},
        {"location", TokenLocation(node->method_declarator->identifier_token)}
    };

    if (node->modifiers_opt)
        result["modifiers"] = SerializeModifiers(node->modifiers_opt);

    if (node->type_parameters_opt)
        result["typeParameters"] = SerializeTypeParameters(node->type_parameters_opt);

    result["declarator"] = SerializeMethodDeclarator(node->method_declarator);

    json throwsTypes = json::array();
    for (unsigned i = 0; i < node->NumThrows(); i++)
        throwsTypes.push_back(SerializeNode(node->Throw(i)));
    if (!throwsTypes.empty())
        result["throws"] = throwsTypes;

    if (node->method_body_opt)
        result["body"] = SerializeMethodBody(node->method_body_opt);

    if (node->default_value_opt)
        result["defaultValue"] = SerializeNode(node->default_value_opt);

    return result;
}

json AstJsonSerializer::SerializeConstructorDeclaration(AstConstructorDeclaration* node)
{
    json result = {
        {"kind", "ConstructorDeclaration"},
        {"name", TokenText(node->constructor_declarator->identifier_token)},
        {"location", TokenLocation(node->constructor_declarator->identifier_token)}
    };

    if (node->modifiers_opt)
        result["modifiers"] = SerializeModifiers(node->modifiers_opt);

    if (node->type_parameters_opt)
        result["typeParameters"] = SerializeTypeParameters(node->type_parameters_opt);

    result["declarator"] = SerializeMethodDeclarator(node->constructor_declarator);

    json throwsTypes = json::array();
    for (unsigned i = 0; i < node->NumThrows(); i++)
        throwsTypes.push_back(SerializeNode(node->Throw(i)));
    if (!throwsTypes.empty())
        result["throws"] = throwsTypes;

    if (node->constructor_body)
        result["body"] = SerializeNode(node->constructor_body);

    return result;
}

json AstJsonSerializer::SerializeInitializerDeclaration(AstInitializerDeclaration* node)
{
    // Check for static modifier via modifiers_opt
    bool isStatic = node->modifiers_opt && node->modifiers_opt->static_token_opt != 0;

    return json{
        {"kind", "InitializerDeclaration"},
        {"isStatic", isStatic},
        {"body", SerializeNode(node->block)},
        {"location", TokenLocation(node->block->left_brace_token)}
    };
}

json AstJsonSerializer::SerializeFormalParameter(AstFormalParameter* node)
{
    json result = {
        {"kind", "FormalParameter"},
        {"type", SerializeNode(node->type)},
        {"name", TokenText(node->formal_declarator->variable_declarator_name->identifier_token)},
        {"isVarargs", node->ellipsis_token_opt != 0},
        {"location", TokenLocation(node->formal_declarator->variable_declarator_name->identifier_token)}
    };

    if (node->modifiers_opt)
        result["modifiers"] = SerializeModifiers(node->modifiers_opt);

    return result;
}

json AstJsonSerializer::SerializeVariableDeclarator(AstVariableDeclarator* node)
{
    json result = {
        {"kind", "VariableDeclarator"},
        {"id", SerializeVariableDeclaratorId(node->variable_declarator_name)},
        {"location", TokenLocation(node->variable_declarator_name->identifier_token)}
    };

    if (node->variable_initializer_opt)
        result["initializer"] = SerializeNode(node->variable_initializer_opt);

    return result;
}

json AstJsonSerializer::SerializeVariableDeclaratorId(AstVariableDeclaratorId* node)
{
    return json{
        {"kind", "VariableDeclaratorId"},
        {"name", TokenText(node->identifier_token)},
        {"dimensions", node->NumBrackets()},
        {"location", TokenLocation(node->identifier_token)}
    };
}

json AstJsonSerializer::SerializeMethodDeclarator(AstMethodDeclarator* node)
{
    json params = json::array();
    for (unsigned i = 0; i < node->NumFormalParameters(); i++)
        params.push_back(SerializeFormalParameter(node->FormalParameter(i)));

    return json{
        {"kind", "MethodDeclarator"},
        {"name", TokenText(node->identifier_token)},
        {"parameters", params},
        {"location", TokenLocation(node->identifier_token)}
    };
}

json AstJsonSerializer::SerializeMethodBody(AstMethodBody* node)
{
    // AstMethodBody extends AstBlock, serialize it as a block
    json result = {
        {"kind", "MethodBody"},
        {"location", TokenLocation(node->left_brace_token)}
    };

    // Serialize statements from the block
    json statements = json::array();
    for (unsigned i = 0; i < node->NumStatements(); i++)
        statements.push_back(SerializeNode(node->Statement(i)));
    result["statements"] = statements;

    // Handle explicit constructor invocation
    if (node->explicit_constructor_opt)
        result["explicitConstructor"] = SerializeNode(node->explicit_constructor_opt);

    return result;
}

// Modifiers and annotations
json AstJsonSerializer::SerializeModifiers(AstModifiers* node)
{
    json keywords = json::array();
    json annotations = json::array();

    for (unsigned i = 0; i < node->NumModifiers(); i++)
    {
        Ast* mod = node->Modifier(i);
        if (mod->kind == Ast::MODIFIER_KEYWORD)
        {
            AstModifierKeyword* keyword = static_cast<AstModifierKeyword*>(mod);
            keywords.push_back(TokenText(keyword->modifier_token));
        }
        else if (mod->kind == Ast::ANNOTATION)
        {
            annotations.push_back(SerializeAnnotation(static_cast<AstAnnotation*>(mod)));
        }
    }

    return json{
        {"kind", "Modifiers"},
        {"keywords", keywords},
        {"annotations", annotations}
    };
}

json AstJsonSerializer::SerializeAnnotation(AstAnnotation* node)
{
    json result = {
        {"kind", "Annotation"},
        {"name", SerializeNode(node->name)},
        {"location", TokenLocation(node->at_token)}
    };

    json pairs = json::array();
    for (unsigned i = 0; i < node->NumMemberValuePairs(); i++)
        pairs.push_back(SerializeMemberValuePair(node->MemberValuePair(i)));
    if (!pairs.empty())
        result["elements"] = pairs;

    return result;
}

json AstJsonSerializer::SerializeMemberValuePair(AstMemberValuePair* node)
{
    json result = {
        {"kind", "MemberValuePair"},
        {"value", SerializeNode(node->member_value)}
    };

    if (node->identifier_token_opt)
        result["name"] = TokenText(node->identifier_token_opt);

    return result;
}

// Statements
json AstJsonSerializer::SerializeBlock(AstBlock* node)
{
    json statements = json::array();
    for (unsigned i = 0; i < node->NumStatements(); i++)
        statements.push_back(SerializeNode(node->Statement(i)));

    return json{
        {"kind", "Block"},
        {"statements", statements},
        {"location", TokenLocation(node->left_brace_token)}
    };
}

json AstJsonSerializer::SerializeLocalVariableStatement(AstLocalVariableStatement* node)
{
    json declarators = json::array();
    for (unsigned i = 0; i < node->NumVariableDeclarators(); i++)
        declarators.push_back(SerializeVariableDeclarator(node->VariableDeclarator(i)));

    json result = {
        {"kind", "LocalVariableStatement"},
        {"type", SerializeNode(node->type)},
        {"declarators", declarators},
        {"location", TokenLocation(node->type->LeftToken())}
    };

    if (node->modifiers_opt)
        result["modifiers"] = SerializeModifiers(node->modifiers_opt);

    return result;
}

json AstJsonSerializer::SerializeLocalClassStatement(AstLocalClassStatement* node)
{
    return json{
        {"kind", "LocalClassStatement"},
        {"declaration", SerializeNode(node->declaration)},
        {"location", TokenLocation(node->declaration->LeftToken())}
    };
}

json AstJsonSerializer::SerializeIfStatement(AstIfStatement* node)
{
    json result = {
        {"kind", "IfStatement"},
        {"condition", SerializeNode(node->expression)},
        {"thenStatement", SerializeNode(node->true_statement)},
        {"location", TokenLocation(node->if_token)}
    };

    if (node->false_statement_opt)
        result["elseStatement"] = SerializeNode(node->false_statement_opt);

    return result;
}

json AstJsonSerializer::SerializeEmptyStatement(AstEmptyStatement* node)
{
    return json{
        {"kind", "EmptyStatement"},
        {"location", TokenLocation(node->semicolon_token)}
    };
}

json AstJsonSerializer::SerializeExpressionStatement(AstExpressionStatement* node)
{
    return json{
        {"kind", "ExpressionStatement"},
        {"expression", SerializeNode(node->expression)},
        {"location", TokenLocation(node->expression->LeftToken())}
    };
}

json AstJsonSerializer::SerializeSwitchStatement(AstSwitchStatement* node)
{
    // switch_block is an AstBlock containing statements
    json body = nullptr;
    if (node->switch_block)
        body = SerializeBlock(node->switch_block);

    return json{
        {"kind", "SwitchStatement"},
        {"expression", SerializeNode(node->expression)},
        {"body", body},
        {"location", TokenLocation(node->switch_token)}
    };
}

json AstJsonSerializer::SerializeSwitchBlockStatement(AstSwitchBlockStatement* node)
{
    json labels = json::array();
    for (unsigned i = 0; i < node->NumSwitchLabels(); i++)
        labels.push_back(SerializeSwitchLabel(node->SwitchLabel(i)));

    json statements = json::array();
    for (unsigned i = 0; i < node->NumStatements(); i++)
        statements.push_back(SerializeNode(node->Statement(i)));

    return json{
        {"kind", "SwitchBlockStatement"},
        {"labels", labels},
        {"statements", statements}
    };
}

json AstJsonSerializer::SerializeSwitchLabel(AstSwitchLabel* node)
{
    json result = {
        {"kind", "SwitchLabel"},
        {"isDefault", node->expression_opt == nullptr},
        {"location", TokenLocation(node->case_token)}
    };

    if (node->expression_opt)
        result["expression"] = SerializeNode(node->expression_opt);

    return result;
}

json AstJsonSerializer::SerializeWhileStatement(AstWhileStatement* node)
{
    return json{
        {"kind", "WhileStatement"},
        {"condition", SerializeNode(node->expression)},
        {"body", SerializeNode(node->statement)},
        {"location", TokenLocation(node->while_token)}
    };
}

json AstJsonSerializer::SerializeDoStatement(AstDoStatement* node)
{
    return json{
        {"kind", "DoStatement"},
        {"body", SerializeNode(node->statement)},
        {"condition", SerializeNode(node->expression)},
        {"location", TokenLocation(node->do_token)}
    };
}

json AstJsonSerializer::SerializeForStatement(AstForStatement* node)
{
    json init = json::array();
    for (unsigned i = 0; i < node->NumForInitStatements(); i++)
        init.push_back(SerializeNode(node->ForInitStatement(i)));

    json update = json::array();
    for (unsigned i = 0; i < node->NumForUpdateStatements(); i++)
        update.push_back(SerializeNode(node->ForUpdateStatement(i)));

    json result = {
        {"kind", "ForStatement"},
        {"init", init},
        {"update", update},
        {"body", SerializeNode(node->statement)},
        {"location", TokenLocation(node->for_token)}
    };

    if (node->end_expression_opt)
        result["condition"] = SerializeNode(node->end_expression_opt);

    return result;
}

json AstJsonSerializer::SerializeForeachStatement(AstForeachStatement* node)
{
    return json{
        {"kind", "ForeachStatement"},
        {"parameter", SerializeFormalParameter(node->formal_parameter)},
        {"expression", SerializeNode(node->expression)},
        {"body", SerializeNode(node->statement)},
        {"location", TokenLocation(node->for_token)}
    };
}

json AstJsonSerializer::SerializeBreakStatement(AstBreakStatement* node)
{
    json result = {
        {"kind", "BreakStatement"},
        {"location", TokenLocation(node->break_token)}
    };

    if (node->identifier_token_opt)
        result["label"] = TokenText(node->identifier_token_opt);

    return result;
}

json AstJsonSerializer::SerializeContinueStatement(AstContinueStatement* node)
{
    json result = {
        {"kind", "ContinueStatement"},
        {"location", TokenLocation(node->continue_token)}
    };

    if (node->identifier_token_opt)
        result["label"] = TokenText(node->identifier_token_opt);

    return result;
}

json AstJsonSerializer::SerializeReturnStatement(AstReturnStatement* node)
{
    json result = {
        {"kind", "ReturnStatement"},
        {"location", TokenLocation(node->return_token)}
    };

    if (node->expression_opt)
        result["expression"] = SerializeNode(node->expression_opt);

    return result;
}

json AstJsonSerializer::SerializeThrowStatement(AstThrowStatement* node)
{
    return json{
        {"kind", "ThrowStatement"},
        {"expression", SerializeNode(node->expression)},
        {"location", TokenLocation(node->throw_token)}
    };
}

json AstJsonSerializer::SerializeSynchronizedStatement(AstSynchronizedStatement* node)
{
    return json{
        {"kind", "SynchronizedStatement"},
        {"expression", SerializeNode(node->expression)},
        {"body", SerializeBlock(node->block)},
        {"location", TokenLocation(node->synchronized_token)}
    };
}

json AstJsonSerializer::SerializeAssertStatement(AstAssertStatement* node)
{
    json result = {
        {"kind", "AssertStatement"},
        {"condition", SerializeNode(node->condition)},
        {"location", TokenLocation(node->assert_token)}
    };

    if (node->message_opt)
        result["message"] = SerializeNode(node->message_opt);

    return result;
}

json AstJsonSerializer::SerializeTryStatement(AstTryStatement* node)
{
    json catches = json::array();
    for (unsigned i = 0; i < node->NumCatchClauses(); i++)
        catches.push_back(SerializeCatchClause(node->CatchClause(i)));

    json result = {
        {"kind", "TryStatement"},
        {"body", SerializeBlock(node->block)},
        {"catches", catches},
        {"location", TokenLocation(node->try_token)}
    };

    if (node->finally_clause_opt)
        result["finally"] = SerializeFinallyClause(node->finally_clause_opt);

    return result;
}

json AstJsonSerializer::SerializeCatchClause(AstCatchClause* node)
{
    return json{
        {"kind", "CatchClause"},
        {"parameter", SerializeFormalParameter(node->formal_parameter)},
        {"body", SerializeBlock(node->block)},
        {"location", TokenLocation(node->catch_token)}
    };
}

json AstJsonSerializer::SerializeFinallyClause(AstFinallyClause* node)
{
    return json{
        {"kind", "FinallyClause"},
        {"body", SerializeBlock(node->block)},
        {"location", TokenLocation(node->finally_token)}
    };
}

// Expressions
json AstJsonSerializer::SerializeExpression(AstExpression* node)
{
    if (!node)
        return nullptr;
    return SerializeNode(node);
}

json AstJsonSerializer::SerializeName(AstName* node)
{
    if (!IsValidPointer(node))
        return nullptr;

    json result = {
        {"kind", "Name"},
        {"identifier", TokenText(node->identifier_token)},
        {"location", TokenLocation(node->identifier_token)}
    };

    if (IsValidPointer(node->base_opt))
        result["base"] = SerializeName(node->base_opt);

    return result;
}

json AstJsonSerializer::SerializeFieldAccess(AstFieldAccess* node)
{
    return json{
        {"kind", "FieldAccess"},
        {"base", SerializeNode(node->base)},
        {"identifier", TokenText(node->identifier_token)},
        {"location", TokenLocation(node->identifier_token)}
    };
}

json AstJsonSerializer::SerializeIntegerLiteral(AstIntegerLiteral* node)
{
    return json{
        {"kind", "IntegerLiteral"},
        {"value", TokenText(node->integer_literal_token)},
        {"location", TokenLocation(node->integer_literal_token)}
    };
}

json AstJsonSerializer::SerializeLongLiteral(AstLongLiteral* node)
{
    return json{
        {"kind", "LongLiteral"},
        {"value", TokenText(node->long_literal_token)},
        {"location", TokenLocation(node->long_literal_token)}
    };
}

json AstJsonSerializer::SerializeFloatLiteral(AstFloatLiteral* node)
{
    return json{
        {"kind", "FloatLiteral"},
        {"value", TokenText(node->float_literal_token)},
        {"location", TokenLocation(node->float_literal_token)}
    };
}

json AstJsonSerializer::SerializeDoubleLiteral(AstDoubleLiteral* node)
{
    return json{
        {"kind", "DoubleLiteral"},
        {"value", TokenText(node->double_literal_token)},
        {"location", TokenLocation(node->double_literal_token)}
    };
}

json AstJsonSerializer::SerializeTrueLiteral(AstTrueLiteral* node)
{
    return json{
        {"kind", "BooleanLiteral"},
        {"value", true},
        {"location", TokenLocation(node->true_literal_token)}
    };
}

json AstJsonSerializer::SerializeFalseLiteral(AstFalseLiteral* node)
{
    return json{
        {"kind", "BooleanLiteral"},
        {"value", false},
        {"location", TokenLocation(node->false_literal_token)}
    };
}

json AstJsonSerializer::SerializeStringLiteral(AstStringLiteral* node)
{
    return json{
        {"kind", "StringLiteral"},
        {"value", TokenText(node->string_literal_token)},
        {"location", TokenLocation(node->string_literal_token)}
    };
}

json AstJsonSerializer::SerializeCharacterLiteral(AstCharacterLiteral* node)
{
    return json{
        {"kind", "CharacterLiteral"},
        {"value", TokenText(node->character_literal_token)},
        {"location", TokenLocation(node->character_literal_token)}
    };
}

json AstJsonSerializer::SerializeNullLiteral(AstNullLiteral* node)
{
    return json{
        {"kind", "NullLiteral"},
        {"location", TokenLocation(node->null_token)}
    };
}

json AstJsonSerializer::SerializeClassLiteral(AstClassLiteral* node)
{
    return json{
        {"kind", "ClassLiteral"},
        {"type", SerializeNode(node->type)},
        {"location", TokenLocation(node->class_token)}
    };
}

json AstJsonSerializer::SerializeThisExpression(AstThisExpression* node)
{
    json result = {
        {"kind", "ThisExpression"},
        {"location", TokenLocation(node->this_token)}
    };

    if (node->base_opt)
        result["qualifier"] = SerializeNode(node->base_opt);

    return result;
}

json AstJsonSerializer::SerializeSuperExpression(AstSuperExpression* node)
{
    json result = {
        {"kind", "SuperExpression"},
        {"location", TokenLocation(node->super_token)}
    };

    if (node->base_opt)
        result["qualifier"] = SerializeNode(node->base_opt);

    return result;
}

json AstJsonSerializer::SerializeParenthesizedExpression(AstParenthesizedExpression* node)
{
    return json{
        {"kind", "ParenthesizedExpression"},
        {"expression", SerializeNode(node->expression)},
        {"location", TokenLocation(node->left_parenthesis_token)}
    };
}

json AstJsonSerializer::SerializeArrayAccess(AstArrayAccess* node)
{
    return json{
        {"kind", "ArrayAccess"},
        {"base", SerializeNode(node->base)},
        {"index", SerializeNode(node->expression)},
        {"location", TokenLocation(node->left_bracket_token)}
    };
}

json AstJsonSerializer::SerializeMethodInvocation(AstMethodInvocation* node)
{
    json result = {
        {"kind", "MethodInvocation"},
        {"location", TokenLocation(node->identifier_token)}
    };

    if (node->base_opt)
        result["base"] = SerializeNode(node->base_opt);

    result["name"] = TokenText(node->identifier_token);

    if (node->type_arguments_opt)
        result["typeArguments"] = SerializeTypeArguments(node->type_arguments_opt);

    if (node->arguments)
        result["arguments"] = SerializeArguments(node->arguments);

    return result;
}

json AstJsonSerializer::SerializeClassCreationExpression(AstClassCreationExpression* node)
{
    json result = {
        {"kind", "ClassCreationExpression"},
        {"type", SerializeNode(node->class_type)},
        {"location", TokenLocation(node->new_token)}
    };

    if (node->base_opt)
        result["base"] = SerializeNode(node->base_opt);

    if (node->type_arguments_opt)
        result["typeArguments"] = SerializeTypeArguments(node->type_arguments_opt);

    if (node->arguments)
        result["arguments"] = SerializeArguments(node->arguments);

    if (node->class_body_opt)
        result["body"] = SerializeClassBody(node->class_body_opt);

    return result;
}

json AstJsonSerializer::SerializeArrayCreationExpression(AstArrayCreationExpression* node)
{
    json dimExprs = json::array();
    for (unsigned i = 0; i < node->NumDimExprs(); i++)
        dimExprs.push_back(SerializeDimExpr(node->DimExpr(i)));

    json result = {
        {"kind", "ArrayCreationExpression"},
        {"type", SerializeNode(node->array_type)},
        {"dimensions", dimExprs},
        {"location", TokenLocation(node->new_token)}
    };

    if (node->array_initializer_opt)
        result["initializer"] = SerializeArrayInitializer(node->array_initializer_opt);

    return result;
}

json AstJsonSerializer::SerializeDimExpr(AstDimExpr* node)
{
    return json{
        {"kind", "DimExpr"},
        {"expression", SerializeNode(node->expression)},
        {"location", TokenLocation(node->left_bracket_token)}
    };
}

json AstJsonSerializer::SerializePostUnaryExpression(AstPostUnaryExpression* node)
{
    const char* op = nullptr;
    switch (node->Tag())
    {
    case AstPostUnaryExpression::PLUSPLUS: op = "++"; break;
    case AstPostUnaryExpression::MINUSMINUS: op = "--"; break;
    default: op = "?"; break;
    }

    return json{
        {"kind", "PostUnaryExpression"},
        {"operator", op},
        {"operand", SerializeNode(node->expression)},
        {"location", TokenLocation(node->post_operator_token)}
    };
}

json AstJsonSerializer::SerializePreUnaryExpression(AstPreUnaryExpression* node)
{
    const char* op = nullptr;
    switch (node->Tag())
    {
    case AstPreUnaryExpression::PLUSPLUS: op = "++"; break;
    case AstPreUnaryExpression::MINUSMINUS: op = "--"; break;
    case AstPreUnaryExpression::PLUS: op = "+"; break;
    case AstPreUnaryExpression::MINUS: op = "-"; break;
    case AstPreUnaryExpression::TWIDDLE: op = "~"; break;
    case AstPreUnaryExpression::NOT: op = "!"; break;
    default: op = "?"; break;
    }

    return json{
        {"kind", "PreUnaryExpression"},
        {"operator", op},
        {"operand", SerializeNode(node->expression)},
        {"location", TokenLocation(node->pre_operator_token)}
    };
}

json AstJsonSerializer::SerializeCastExpression(AstCastExpression* node)
{
    return json{
        {"kind", "CastExpression"},
        {"type", SerializeNode(node->type)},
        {"expression", SerializeNode(node->expression)},
        {"location", TokenLocation(node->left_parenthesis_token)}
    };
}

json AstJsonSerializer::SerializeBinaryExpression(AstBinaryExpression* node)
{
    const char* op = nullptr;
    switch (node->Tag())
    {
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
    default: op = "?"; break;
    }

    return json{
        {"kind", "BinaryExpression"},
        {"operator", op},
        {"left", SerializeNode(node->left_expression)},
        {"right", SerializeNode(node->right_expression)},
        {"location", TokenLocation(node->binary_operator_token)}
    };
}

json AstJsonSerializer::SerializeInstanceofExpression(AstInstanceofExpression* node)
{
    return json{
        {"kind", "InstanceofExpression"},
        {"expression", SerializeNode(node->expression)},
        {"type", SerializeNode(node->type)},
        {"location", TokenLocation(node->instanceof_token)}
    };
}

json AstJsonSerializer::SerializeConditionalExpression(AstConditionalExpression* node)
{
    return json{
        {"kind", "ConditionalExpression"},
        {"condition", SerializeNode(node->test_expression)},
        {"thenExpression", SerializeNode(node->true_expression)},
        {"elseExpression", SerializeNode(node->false_expression)},
        {"location", TokenLocation(node->question_token)}
    };
}

json AstJsonSerializer::SerializeAssignmentExpression(AstAssignmentExpression* node)
{
    const char* op = nullptr;
    switch (node->Tag())
    {
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
    default: op = "?"; break;
    }

    return json{
        {"kind", "AssignmentExpression"},
        {"operator", op},
        {"left", SerializeNode(node->left_hand_side)},
        {"right", SerializeNode(node->expression)},
        {"location", TokenLocation(node->assignment_operator_token)}
    };
}

json AstJsonSerializer::SerializeArrayInitializer(AstArrayInitializer* node)
{
    json elements = json::array();
    for (unsigned i = 0; i < node->NumVariableInitializers(); i++)
        elements.push_back(SerializeNode(node->VariableInitializer(i)));

    return json{
        {"kind", "ArrayInitializer"},
        {"elements", elements},
        {"location", TokenLocation(node->left_brace_token)}
    };
}

json AstJsonSerializer::SerializeThisCall(AstThisCall* node)
{
    json result = {
        {"kind", "ThisCall"},
        {"location", TokenLocation(node->this_token)}
    };

    if (node->type_arguments_opt)
        result["typeArguments"] = SerializeTypeArguments(node->type_arguments_opt);

    if (node->arguments)
        result["arguments"] = SerializeArguments(node->arguments);

    return result;
}

json AstJsonSerializer::SerializeSuperCall(AstSuperCall* node)
{
    json result = {
        {"kind", "SuperCall"},
        {"location", TokenLocation(node->super_token)}
    };

    if (node->base_opt)
        result["base"] = SerializeNode(node->base_opt);

    if (node->type_arguments_opt)
        result["typeArguments"] = SerializeTypeArguments(node->type_arguments_opt);

    if (node->arguments)
        result["arguments"] = SerializeArguments(node->arguments);

    return result;
}

json AstJsonSerializer::SerializeArguments(AstArguments* node)
{
    json args = json::array();
    for (unsigned i = 0; i < node->NumArguments(); i++)
        args.push_back(SerializeNode(node->Argument(i)));

    return json{
        {"kind", "Arguments"},
        {"arguments", args}
    };
}

json AstJsonSerializer::SerializeBrackets(AstBrackets* node)
{
    return json{
        {"kind", "Brackets"},
        {"location", TokenLocation(node->left_bracket_token)}
    };
}

json AstJsonSerializer::SerializeLexErrors(LexStream* lex_stream)
{
    if (!lex_stream)
        return json::array();

    json errors = json::array();
    for (unsigned i = 0; i < lex_stream->NumLexErrors(); i++)
    {
        StreamError& err = lex_stream->LexError(i);
        json error_entry;

        // Get severity
        const char* severity;
        switch (err.getSeverity())
        {
            case JopaError::JOPA_ERROR:
                severity = "error";
                break;
            case JopaError::JOPA_CAUTION:
                severity = "caution";
                break;
            case JopaError::JOPA_WARNING:
                severity = "warning";
                break;
            default:
                severity = "unknown";
        }

        error_entry["severity"] = severity;

        // Only include location if error was already initialized
        // (initialization requires input buffer which may be destroyed)
        if (err.IsInitialized())
        {
            error_entry["location"] = json{
                {"startLine", err.getLeftLineNo()},
                {"startColumn", err.getLeftColumnNo()},
                {"endLine", err.getRightLineNo()},
                {"endColumn", err.getRightColumnNo()}
            };
        }

        // Get the error message, converting wide string to UTF-8
        const wchar_t* msg = err.getErrorMessage();
        if (msg)
        {
            std::string message;
            for (const wchar_t* p = msg; *p; ++p)
            {
                uint32_t ch = static_cast<uint32_t>(*p);

                // Handle UTF-16 surrogate pairs
                if (ch >= 0xD800 && ch <= 0xDBFF && p[1] >= 0xDC00 && p[1] <= 0xDFFF)
                {
                    uint32_t low = static_cast<uint32_t>(p[1]);
                    ch = ((ch - 0xD800) << 10) + (low - 0xDC00) + 0x10000;
                    ++p;
                }
                else if ((ch >= 0xD800 && ch <= 0xDBFF) || (ch >= 0xDC00 && ch <= 0xDFFF))
                {
                    ch = 0xFFFD; // Replacement character for lone surrogates
                }

                // Encode as UTF-8
                if (ch < 0x80)
                    message += static_cast<char>(ch);
                else if (ch < 0x800)
                {
                    message += static_cast<char>(0xC0 | (ch >> 6));
                    message += static_cast<char>(0x80 | (ch & 0x3F));
                }
                else if (ch < 0x10000)
                {
                    message += static_cast<char>(0xE0 | (ch >> 12));
                    message += static_cast<char>(0x80 | ((ch >> 6) & 0x3F));
                    message += static_cast<char>(0x80 | (ch & 0x3F));
                }
                else
                {
                    message += static_cast<char>(0xF0 | (ch >> 18));
                    message += static_cast<char>(0x80 | ((ch >> 12) & 0x3F));
                    message += static_cast<char>(0x80 | ((ch >> 6) & 0x3F));
                    message += static_cast<char>(0x80 | (ch & 0x3F));
                }
            }
            error_entry["message"] = message;
        }

        errors.push_back(error_entry);
    }

    return errors;
}

} // namespace Jopa
