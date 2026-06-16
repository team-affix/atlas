#include "../hpp/clause_visitor.hpp"
#include "infrastructure/non_backtracking_var_sequencer.hpp"

clause_visitor::clause_visitor(i_make_functor& make_functor, i_make_var& make_var,
                               std::map<std::string, uint32_t>& functor_map, uint32_t& next_functor_id)
    : make_functor(make_functor),
      make_var(make_var),
      functor_map(functor_map),
      next_functor_id(next_functor_id) {}

antlrcpp::Any clause_visitor::visitClause(CHCParser::ClauseContext* ctx) {
    std::map<std::string, uint32_t> var_map;
    non_backtracking_var_sequencer local_seq(0);
    expr_visitor ev(make_functor, make_var, local_seq, var_map, functor_map, next_functor_id);

    const expr* head = std::any_cast<const expr*>(ev.visitExpr(ctx->expr()));

    auto* b = ctx->body();
    if (!b) return rule{head, {}, local_seq.peek()};

    body_visitor bv(make_functor, make_var, local_seq, var_map, functor_map, next_functor_id);
    auto body = std::any_cast<std::vector<const expr*>>(bv.visitBody(b));
    return rule{head, std::move(body), local_seq.peek()};
}
