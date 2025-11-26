#include "parser_facade.h"
#include "control.h"
#include "parser.h"
#include "scanner.h"
#include "ast.h"
#include "stream.h"
#include "ast_serializer.h"
#include "jast.h"

namespace Jopa {

// ParseError implementation
nlohmann::json ParseError::toJson() const {
    return {
        {"line", line},
        {"column", column},
        {"message", message},
        {"severity", severity}
    };
}

ParseError ParseError::fromJson(const nlohmann::json& j) {
    ParseError err;
    err.line = j.value("line", 0);
    err.column = j.value("column", 0);
    err.message = j.value("message", "");
    err.severity = j.value("severity", "error");
    return err;
}

// ParseResult implementation
nlohmann::json ParseResult::toJson() const {
    nlohmann::json result;
    result["success"] = success;

    nlohmann::json errorArray = nlohmann::json::array();
    for (const auto& err : errors) {
        errorArray.push_back(err.toJson());
    }
    result["errors"] = errorArray;

    result["ast"] = ast;
    return result;
}

ParseResult ParseResult::fromJson(const nlohmann::json& j) {
    ParseResult result;
    result.success = j.value("success", false);

    if (j.contains("errors") && j["errors"].is_array()) {
        for (const auto& err : j["errors"]) {
            result.errors.push_back(ParseError::fromJson(err));
        }
    }

    if (j.contains("ast")) {
        result.ast = j["ast"];
    }

    return result;
}

// LegacyParserAdapter implementation
LegacyParserAdapter::LegacyParserAdapter(Control& control)
    : control_(control)
{
}

LegacyParserAdapter::~LegacyParserAdapter() = default;

ParseResult LegacyParserAdapter::parseCompilationUnit(FileSymbol* file_symbol) {
    ParseResult result;
    result.success = false;

    if (!file_symbol) {
        ParseError err;
        err.line = 0;
        err.column = 0;
        err.message = "No file symbol provided";
        err.severity = "error";
        result.errors.push_back(err);
        return result;
    }

    LexStream* lex_stream = file_symbol->lex_stream;
    if (!lex_stream) {
        ParseError err;
        err.line = 0;
        err.column = 0;
        err.message = "No lex stream available for file";
        err.severity = "error";
        result.errors.push_back(err);
        return result;
    }

    // Phase 1: Header parse
    // Let HeaderParse create its own body_pool which gets stored in compilation_unit->ast_pool
    lex_stream->Reset();
    AstCompilationUnit* compilation_unit =
        control_.parser->HeaderParse(lex_stream);

    if (!compilation_unit) {
        ParseError err;
        err.line = 1;
        err.column = 1;
        err.message = "Failed to parse compilation unit";
        err.severity = "error";
        result.errors.push_back(err);
        return result;
    }

    // Store compilation_unit in file_symbol for cleanup (even on error)
    file_symbol->compilation_unit = compilation_unit;

    // Check for header parse errors
    if (compilation_unit->BadCompilationUnitCast()) {
        collectLexErrors(lex_stream, result.errors);
        return result;
    }

    // Phase 2: Body parse for all type declarations
    bool body_parse_success = true;
    for (unsigned i = 0; i < compilation_unit->NumTypeDeclarations(); i++) {
        AstDeclaredType* type_decl = compilation_unit->TypeDeclaration(i);
        if (type_decl && type_decl->class_body) {
            // Only parse if the body is unparsed (some bodies like empty enums are already parsed)
            if (type_decl->class_body->UnparsedClassBodyCast()) {
                // Parse initializers
                if (!control_.parser->InitializerParse(lex_stream, type_decl->class_body)) {
                    body_parse_success = false;
                    break;
                }

                // Parse method bodies
                if (!control_.parser->BodyParse(lex_stream, type_decl->class_body)) {
                    body_parse_success = false;
                    break;
                }
            }

            // Recursively parse nested types
            if (!parseNestedBodies(lex_stream, type_decl->class_body)) {
                body_parse_success = false;
                break;
            }
        }
    }

    if (!body_parse_success) {
        collectLexErrors(lex_stream, result.errors);
        return result;
    }

    // Collect any lexer errors/warnings
    collectLexErrors(lex_stream, result.errors);

    // Serialize the complete AST to JSON
    AstSerializer serializer(*lex_stream);
    result.ast = serializer.serialize(compilation_unit);
    result.success = true;

    return result;
}

std::optional<nlohmann::json> LegacyParserAdapter::parsePackageOnly(FileSymbol* file_symbol) {
    if (!file_symbol || !file_symbol->lex_stream) {
        return std::nullopt;
    }

    StoragePool* ast_pool = new StoragePool(64);
    AstPackageDeclaration* package_decl =
        control_.parser->PackageHeaderParse(file_symbol->lex_stream, ast_pool);

    if (!package_decl) {
        delete ast_pool;
        return std::nullopt;
    }

    AstSerializer serializer(*file_symbol->lex_stream);
    nlohmann::json result = serializer.serialize(package_decl);

    delete ast_pool;
    return result;
}

bool LegacyParserAdapter::parseNestedBodies(LexStream* lex_stream, AstClassBody* class_body) {
    if (!class_body) return true;

    // Parse nested classes
    for (unsigned i = 0; i < class_body->NumNestedClasses(); i++) {
        AstClassDeclaration* nested = class_body->NestedClass(i);
        if (nested && nested->class_body) {
            if (nested->class_body->UnparsedClassBodyCast()) {
                if (!control_.parser->InitializerParse(lex_stream, nested->class_body)) {
                    return false;
                }
                if (!control_.parser->BodyParse(lex_stream, nested->class_body)) {
                    return false;
                }
            }
            if (!parseNestedBodies(lex_stream, nested->class_body)) {
                return false;
            }
        }
    }

    // Parse nested enums
    for (unsigned i = 0; i < class_body->NumNestedEnums(); i++) {
        AstEnumDeclaration* nested = class_body->NestedEnum(i);
        if (nested && nested->class_body) {
            if (nested->class_body->UnparsedClassBodyCast()) {
                if (!control_.parser->InitializerParse(lex_stream, nested->class_body)) {
                    return false;
                }
                if (!control_.parser->BodyParse(lex_stream, nested->class_body)) {
                    return false;
                }
            }
            if (!parseNestedBodies(lex_stream, nested->class_body)) {
                return false;
            }
        }
    }

    // Parse nested interfaces
    for (unsigned i = 0; i < class_body->NumNestedInterfaces(); i++) {
        AstInterfaceDeclaration* nested = class_body->NestedInterface(i);
        if (nested && nested->class_body) {
            if (nested->class_body->UnparsedClassBodyCast()) {
                if (!control_.parser->InitializerParse(lex_stream, nested->class_body)) {
                    return false;
                }
                if (!control_.parser->BodyParse(lex_stream, nested->class_body)) {
                    return false;
                }
            }
            if (!parseNestedBodies(lex_stream, nested->class_body)) {
                return false;
            }
        }
    }

    // Parse nested annotations
    for (unsigned i = 0; i < class_body->NumNestedAnnotations(); i++) {
        AstAnnotationDeclaration* nested = class_body->NestedAnnotation(i);
        if (nested && nested->class_body) {
            if (nested->class_body->UnparsedClassBodyCast()) {
                if (!control_.parser->InitializerParse(lex_stream, nested->class_body)) {
                    return false;
                }
                if (!control_.parser->BodyParse(lex_stream, nested->class_body)) {
                    return false;
                }
            }
            if (!parseNestedBodies(lex_stream, nested->class_body)) {
                return false;
            }
        }
    }

    return true;
}

void LegacyParserAdapter::collectLexErrors(LexStream* lex_stream, std::vector<ParseError>& errors) {
    // The lex stream has bad_tokens that contain lexer errors
    // We iterate through them and collect the errors
    // Note: The actual error messages are obtained through the StreamError interface

    unsigned num_bad = lex_stream->NumBadTokens();
    unsigned num_warn = lex_stream->NumWarnTokens();

    if (num_bad > 0 || num_warn > 0) {
        // For simplicity, we'll add a generic error if there are bad tokens
        // A more complete implementation would extract individual error messages
        if (num_bad > 0) {
            ParseError err;
            err.line = 1;
            err.column = 1;
            err.message = "Lexical errors detected in source file";
            err.severity = "error";
            errors.push_back(err);
        }
    }
}

bool LegacyParserAdapter::initializerParse(LexStream* lex_stream, AstClassBody* class_body) {
    return control_.parser->InitializerParse(lex_stream, class_body);
}

bool LegacyParserAdapter::bodyParse(LexStream* lex_stream, AstClassBody* class_body) {
    return control_.parser->BodyParse(lex_stream, class_body);
}

AstPackageDeclaration* LegacyParserAdapter::packageHeaderParse(LexStream* lex_stream, StoragePool* ast_pool) {
    return control_.parser->PackageHeaderParse(lex_stream, ast_pool);
}

AstCompilationUnit* LegacyParserAdapter::headerParse(LexStream* lex_stream) {
    return control_.parser->HeaderParse(lex_stream);
}

// Factory function
std::unique_ptr<IParserFacade> createParserFacade(Control& control) {
    return std::make_unique<LegacyParserAdapter>(control);
}

} // namespace Jopa
