#include "../hpp/clause_visitor.hpp"

clause_visitor::clause_visitor(i_make_functor& make_functor, i_make_var& make_var, i_var_sequencer& var_seq,
                               std::map<std::string, uint32_t>& atom_map, uint32_t& next_atom_id)
    : make_functor(make_functor),
      make_var(make_var),
      var_seq(var_seq),
      atom_map(atom_map),
      next_atom_id(next_atom_id) {}

antlrcpp::Any clause_visitor::visitClause(CHCParser::ClauseContext* ctx) {
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev(make_functor, make_var, var_seq, var_map, atom_map, next_atom_id);

    const expr* head = std::any_cast<const expr*>(ev.visitExpr(ctx->expr()));

    auto* b = ctx->body();
    if (!b) return rule{head, {}};

    body_visitor bv(make_functor, make_var, var_seq, var_map, atom_map, next_atom_id);
    auto body = std::any_cast<std::vector<const expr*>>(bv.visitBody(b));
    return rule{head, body};
}
