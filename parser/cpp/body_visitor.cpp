#include "../hpp/body_visitor.hpp"

body_visitor::body_visitor(i_make_functor& make_functor, i_make_var& make_var, i_var_sequencer& var_seq,
                           std::map<std::string, uint32_t>& var_map)
    : make_functor(make_functor), make_var(make_var), var_seq(var_seq), var_map(var_map) {}

antlrcpp::Any body_visitor::visitBody(CHCParser::BodyContext* ctx) {
    expr_visitor ev(make_functor, make_var, var_seq, var_map);
    std::vector<const expr*> body;
    for (auto* e : ctx->expr())
        body.push_back(std::any_cast<const expr*>(ev.visitExpr(e)));
    return body;
}
