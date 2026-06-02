#include "../hpp/database_visitor.hpp"
#include "../hpp/clause_visitor.hpp"

database_visitor::database_visitor(i_make_functor& make_functor, i_make_var& make_var, i_var_sequencer& var_seq,
                                   i_push_db_rule& out)
    : make_functor(make_functor), make_var(make_var), var_seq(var_seq), out(out) {}

antlrcpp::Any database_visitor::visitDatabase(CHCParser::DatabaseContext* ctx) {
    clause_visitor cv(make_functor, make_var, var_seq);
    for (auto* c : ctx->clause())
        out.push(std::any_cast<rule>(cv.visitClause(c)));
    return antlrcpp::Any();
}
