#include "../hpp/body_visitor.hpp"

body_visitor::body_visitor(i_make_functor& make_functor, i_make_var& make_var, i_var_sequencer& var_seq,
                           std::map<std::string, uint32_t>& var_map,
                           std::map<std::string, uint32_t>& atom_map, uint32_t& next_atom_id)
    : make_functor(make_functor),
      make_var(make_var),
      var_seq(var_seq),
      var_map(var_map),
      atom_map(atom_map),
      next_atom_id(next_atom_id) {}

antlrcpp::Any body_visitor::visitBody(CHCParser::BodyContext* ctx) {
    std::vector<const expr*> result;
    expr_visitor ev(make_functor, make_var, var_seq, var_map, atom_map, next_atom_id);
    for (auto* e : ctx->expr())
        result.push_back(std::any_cast<const expr*>(ev.visit(e)));
    return result;
}
