#include "../hpp/import_goals_from_string.hpp"
#include "../hpp/body_visitor.hpp"
#include "../generated/CHCLexer.h"
#include "../generated/CHCParser.h"
#include <stdexcept>

parsed_goals import_goals_from_string(const std::string& body, expr_pool& pool, sequencer& seq) {
    antlr4::ANTLRInputStream stream(body);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    parser.removeErrorListeners();

    auto* ctx = parser.body();
    if (parser.getNumberOfSyntaxErrors() > 0)
        throw std::runtime_error("parse error in goal string: " + body);

    std::map<std::string, uint32_t> var_name_to_idx;
    body_visitor bv(pool, seq, var_name_to_idx);
    goals gl = std::any_cast<goals>(bv.visitBody(ctx));
    return parsed_goals{std::move(gl), std::move(var_name_to_idx)};
}
