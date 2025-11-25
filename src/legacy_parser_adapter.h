#ifndef legacy_parser_adapter_INCLUDED
#define legacy_parser_adapter_INCLUDED

#include "parser_interface.h"

namespace Jopa {

class Parser;

// Adapter that wraps the legacy Parser to implement IParser interface.
// This allows gradual migration away from the legacy parser.
class LegacyParserAdapter : public IParser
{
public:
    LegacyParserAdapter();
    ~LegacyParserAdapter() override;

    // Full parsing (for --parse-only mode)
    ParseResult ParseFile(LexStream* lex_stream) override;
    BatchParseResult ParseFiles(std::vector<LexStream*>& streams) override;

    // Phased parsing (for incremental migration)
    AstPackageDeclaration* ParsePackageHeader(LexStream* lex_stream,
                                               StoragePool* pool) override;
    AstCompilationUnit* ParseHeaders(LexStream* lex_stream,
                                      StoragePool* pool = nullptr) override;
    bool ParseInitializers(LexStream* lex_stream,
                           AstClassBody* class_body) override;
    bool ParseBodies(LexStream* lex_stream,
                     AstClassBody* class_body) override;
    bool ParseAllBodies(LexStream* lex_stream,
                        AstClassBody* class_body) override;

private:
    Parser* parser_;
};

} // namespace Jopa

#endif // legacy_parser_adapter_INCLUDED
