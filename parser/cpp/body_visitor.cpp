#include "../hpp/body_visitor.hpp"

body_visitor::body_visitor(expr_pool& pool, sequencer& seq, std::map<std::string, uint32_t>& var_map)
    : pool(pool), seq(seq), var_map(var_map) {}

antlrcpp::Any body_visitor::visitBody(CHCParser::BodyContext* ctx) {
    expr_visitor ev(pool, seq, var_map);
    std::vector<const expr*> body;
    for (auto* e : ctx->expr())
        body.push_back(std::any_cast<const expr*>(ev.visitExpr(e)));
    return body;
}
