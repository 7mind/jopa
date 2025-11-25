#ifndef parser_facade_INCLUDED
#define parser_facade_INCLUDED

#include "platform.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace Jopa {

class LexStream;
class StoragePool;
class Control;
class FileSymbol;

// Forward declarations for JAST (JSON-based AST)
namespace jast {
class CompilationUnit;
class ClassBody;
class PackageDeclaration;
}

// Represents a single parse error in JSON format
struct ParseError {
    int line;
    int column;
    std::string message;
    std::string severity; // "error" or "warning"

    nlohmann::json toJson() const;
    static ParseError fromJson(const nlohmann::json& j);
};

// Represents the result of parsing a source file
struct ParseResult {
    bool success;
    std::vector<ParseError> errors;
    nlohmann::json ast; // The JSON representation of the AST

    nlohmann::json toJson() const;
    static ParseResult fromJson(const nlohmann::json& j);
};

class AstClassBody;
class AstPackageDeclaration;
class AstCompilationUnit;

// Abstract interface for parsing Java source files
// This interface decouples the rest of the compiler from the legacy parser
class IParserFacade {
public:
    virtual ~IParserFacade() = default;

    // Parse a complete compilation unit (package + headers + bodies)
    // Returns JSON representation of the AST
    virtual ParseResult parseCompilationUnit(FileSymbol* file_symbol) = 0;

    // Parse only the package declaration (for initial package resolution)
    virtual std::optional<nlohmann::json> parsePackageOnly(FileSymbol* file_symbol) = 0;

    // Parse initializers for an unparsed class body (for anonymous classes)
    virtual bool initializerParse(LexStream* lex_stream, AstClassBody* class_body) = 0;

    // Parse method bodies for an unparsed class body (for anonymous classes)
    virtual bool bodyParse(LexStream* lex_stream, AstClassBody* class_body) = 0;

    // Parse just the package header and return legacy AST (for incremental compilation)
    virtual AstPackageDeclaration* packageHeaderParse(LexStream* lex_stream, StoragePool* ast_pool) = 0;

    // Parse type headers (class/interface declarations) and return legacy AST
    virtual AstCompilationUnit* headerParse(LexStream* lex_stream) = 0;
};

// Legacy parser adapter that wraps the existing Parser class
// This adapter performs full parsing and serializes the result to JSON
class LegacyParserAdapter : public IParserFacade {
public:
    explicit LegacyParserAdapter(Control& control);
    ~LegacyParserAdapter() override;

    ParseResult parseCompilationUnit(FileSymbol* file_symbol) override;
    std::optional<nlohmann::json> parsePackageOnly(FileSymbol* file_symbol) override;
    bool initializerParse(LexStream* lex_stream, AstClassBody* class_body) override;
    bool bodyParse(LexStream* lex_stream, AstClassBody* class_body) override;
    AstPackageDeclaration* packageHeaderParse(LexStream* lex_stream, StoragePool* ast_pool) override;
    AstCompilationUnit* headerParse(LexStream* lex_stream) override;

private:
    Control& control_;

    // Recursively parse all nested class bodies
    bool parseNestedBodies(LexStream* lex_stream, AstClassBody* class_body);

    // Collect lexer errors and warnings
    void collectLexErrors(LexStream* lex_stream, std::vector<ParseError>& errors);
};

// Factory function to create the appropriate parser facade
std::unique_ptr<IParserFacade> createParserFacade(Control& control);

} // namespace Jopa

#endif // parser_facade_INCLUDED
