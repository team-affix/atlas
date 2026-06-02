#include <stdexcept>
#include "../hpp/import_goals_from_string.hpp"
#include "../hpp/body_visitor.hpp"
#include "../generated/CHCLexer.h"
#include "../generated/CHCParser.h"

std::map<std::string, uint32_t> import_goals_from_string(const std::string& body, i_make_functor& make_functor,
                                                         i_make_var& make_var, i_var_sequencer& var_seq,
                                                         i_push_initial_goal_expr& goals) {
    antlr4::ANTLRInputStream stream(body);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    parser.removeErrorListeners();

    auto* ctx = parser.body();
    if (parser.getNumberOfSyntaxErrors() > 0)
        throw std::runtime_error("parse error in goal string: " + body);

    std::map<std::string, uint32_t> var_name_to_idx;
    body_visitor bv(make_functor, make_var, var_seq, var_name_to_idx);
    auto gl = std::any_cast<std::vector<const expr*>>(bv.visitBody(ctx));
    for (const expr* e : gl)
        goals.push(e);
    return var_name_to_idx;
}
