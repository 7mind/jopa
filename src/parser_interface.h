#ifndef parser_interface_INCLUDED
#define parser_interface_INCLUDED

#include "platform.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace Jopa {

using json = nlohmann::json;

class LexStream;
class StoragePool;
class AstCompilationUnit;
class AstPackageDeclaration;
class AstClassBody;

// Result of parsing a single file
struct ParseResult
{
    bool success;
    int error_count;
    json ast_json;
    json lex_errors;
    std::string filename;

    // Legacy AST for backward compatibility during transition.
    // Will be removed once compiler is fully decoupled.
    AstCompilationUnit* legacy_ast;
    StoragePool* legacy_ast_pool;

    ParseResult()
        : success(false)
        , error_count(0)
        , legacy_ast(nullptr)
        , legacy_ast_pool(nullptr)
    {}
};

// Result of parsing multiple files
struct BatchParseResult
{
    bool overall_success;
    int total_errors;
    std::vector<ParseResult> file_results;
    std::vector<std::string> io_errors;
};

// Abstract interface for parser implementations.
// This allows the compiler to be decoupled from the specific parser implementation.
class IParser
{
public:
    virtual ~IParser() = default;

    // ========== Full Parsing (for --parse-only mode) ==========

    // Parse a single file completely (headers + bodies).
    // Returns a ParseResult with both JSON AST and legacy AST (for transition).
    virtual ParseResult ParseFile(LexStream* lex_stream) = 0;

    // Parse multiple files.
    virtual BatchParseResult ParseFiles(std::vector<LexStream*>& streams) = 0;

    // ========== Phased Parsing (for incremental migration) ==========

    // Phase 0: Parse just the package declaration.
    // Returns the package declaration AST (still legacy, for now).
    virtual AstPackageDeclaration* ParsePackageHeader(LexStream* lex_stream,
                                                       StoragePool* pool) = 0;

    // Phase 1: Parse type declarations (headers only, no method bodies).
    // Returns the compilation unit AST (still legacy, for now).
    virtual AstCompilationUnit* ParseHeaders(LexStream* lex_stream,
                                              StoragePool* pool = nullptr) = 0;

    // Phase 2a: Parse initializers for a class body.
    // Returns true on success.
    virtual bool ParseInitializers(LexStream* lex_stream,
                                   AstClassBody* class_body) = 0;

    // Phase 2b: Parse method bodies for a class body (requires semantic analysis first).
    // Returns true on success.
    virtual bool ParseBodies(LexStream* lex_stream,
                             AstClassBody* class_body) = 0;

    // Phase 2 (combined): Parse all bodies without semantic analysis.
    // Used when full validation is needed without compilation.
    virtual bool ParseAllBodies(LexStream* lex_stream,
                                AstClassBody* class_body) = 0;
};

} // namespace Jopa

#endif // parser_interface_INCLUDED
