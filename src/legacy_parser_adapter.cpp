#include "legacy_parser_adapter.h"
#include "parser.h"
#include "ast.h"
#include "ast_json.h"
#include "stream.h"

namespace Jopa {

LegacyParserAdapter::LegacyParserAdapter()
    : parser_(new Parser())
{
}

LegacyParserAdapter::~LegacyParserAdapter()
{
    delete parser_;
}

ParseResult LegacyParserAdapter::ParseFile(LexStream* lex_stream)
{
    ParseResult result;
    result.filename = lex_stream->FileName();

    lex_stream->Reset();

    // Phase 1: Header parsing
    AstCompilationUnit* compilation_unit = parser_->HeaderParse(lex_stream);

    if (!compilation_unit || compilation_unit->BadCompilationUnitCast())
    {
        result.success = false;
        result.error_count = lex_stream->NumBadTokens();
        result.lex_errors = AstJsonSerializer::SerializeLexErrors(lex_stream);
        result.legacy_ast = compilation_unit;
        result.legacy_ast_pool = compilation_unit ? compilation_unit->ast_pool : nullptr;
        return result;
    }

    // Phase 2: Body parsing for all type declarations
    bool body_parse_success = true;
    for (unsigned t = 0; t < compilation_unit->NumTypeDeclarations(); t++)
    {
        AstDeclaredType* type_decl = compilation_unit->TypeDeclaration(t);
        if (type_decl->class_body && type_decl->class_body->UnparsedClassBodyCast())
        {
            if (!parser_->FullBodyParse(lex_stream, type_decl->class_body))
            {
                body_parse_success = false;
            }
        }
    }

    result.success = body_parse_success && (lex_stream->NumBadTokens() == 0);
    result.error_count = lex_stream->NumBadTokens();
    result.lex_errors = AstJsonSerializer::SerializeLexErrors(lex_stream);

    // Serialize to JSON
    AstJsonSerializer serializer(lex_stream);
    result.ast_json = serializer.Serialize(compilation_unit);

    // Keep legacy AST for backward compatibility
    result.legacy_ast = compilation_unit;
    result.legacy_ast_pool = compilation_unit->ast_pool;

    return result;
}

BatchParseResult LegacyParserAdapter::ParseFiles(std::vector<LexStream*>& streams)
{
    BatchParseResult batch_result;
    batch_result.overall_success = true;
    batch_result.total_errors = 0;

    for (LexStream* stream : streams)
    {
        ParseResult file_result = ParseFile(stream);

        batch_result.total_errors += file_result.error_count;
        if (!file_result.success)
        {
            batch_result.overall_success = false;
        }

        batch_result.file_results.push_back(std::move(file_result));
    }

    return batch_result;
}

AstPackageDeclaration* LegacyParserAdapter::ParsePackageHeader(LexStream* lex_stream,
                                                                StoragePool* pool)
{
    return parser_->PackageHeaderParse(lex_stream, pool);
}

AstCompilationUnit* LegacyParserAdapter::ParseHeaders(LexStream* lex_stream,
                                                       StoragePool* pool)
{
    return parser_->HeaderParse(lex_stream, pool);
}

bool LegacyParserAdapter::ParseInitializers(LexStream* lex_stream,
                                             AstClassBody* class_body)
{
    return parser_->InitializerParse(lex_stream, class_body);
}

bool LegacyParserAdapter::ParseBodies(LexStream* lex_stream,
                                       AstClassBody* class_body)
{
    return parser_->BodyParse(lex_stream, class_body);
}

bool LegacyParserAdapter::ParseAllBodies(LexStream* lex_stream,
                                          AstClassBody* class_body)
{
    return parser_->FullBodyParse(lex_stream, class_body);
}

} // namespace Jopa
