#ifndef IMPORT_GOALS_FROM_STRING_HPP
#define IMPORT_GOALS_FROM_STRING_HPP

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include "../generated/CHCLexer.h"
#include "../generated/CHCParser.h"
#include "../hpp/body_visitor.hpp"

template<typename IMakeFunctor, typename IMakeVar, typename IVarSequencer, typename IPushInitialGoalExpr>
std::map<std::string, uint32_t> import_goals_from_string(
    const std::string& body,
    IMakeFunctor& make_functor,
    IMakeVar& make_var,
    IVarSequencer& var_seq,
    IPushInitialGoalExpr& goals,
    std::map<std::string, uint32_t>& functor_map,
    uint32_t& next_functor_id) {
    antlr4::ANTLRInputStream stream(body);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    parser.removeErrorListeners();

    auto* ctx = parser.body();
    if (parser.getNumberOfSyntaxErrors() > 0)
        throw std::runtime_error("parse error in goal string: " + body);

    std::map<std::string, uint32_t> var_name_to_idx;
    body_visitor bv(make_functor, make_var, var_seq, var_name_to_idx, functor_map, next_functor_id);
    auto gl = std::any_cast<std::vector<const expr*>>(bv.visitBody(ctx));
    for (const expr* e : gl)
        goals.push(e);
    return var_name_to_idx;
}

#endif
