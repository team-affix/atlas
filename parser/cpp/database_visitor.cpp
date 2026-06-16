#include "../hpp/database_visitor.hpp"
#include "../hpp/clause_visitor.hpp"

database_visitor::database_visitor(i_make_functor& make_functor, i_make_var& make_var,
                                   i_push_db_rule& out, std::map<std::string, uint32_t>& functor_map,
                                   uint32_t& next_functor_id)
    : make_functor(make_functor),
      make_var(make_var),
      out(out),
      functor_map(functor_map),
      next_functor_id(next_functor_id) {}

antlrcpp::Any database_visitor::visitDatabase(CHCParser::DatabaseContext* ctx) {
    clause_visitor cv(make_functor, make_var, functor_map, next_functor_id);
    for (auto* c : ctx->clause())
        out.push(std::any_cast<rule>(cv.visitClause(c)));
    return antlrcpp::Any();
}
