#include "../hpp/body_visitor.hpp"
#include "../hpp/pred_visitor.hpp"

body_visitor::body_visitor(expr_pool& pool, sequencer& seq, std::map<std::string, uint32_t>& var_map)
    : pool(pool), seq(seq), var_map(var_map) {}

antlrcpp::Any body_visitor::visitBody(CHCParser::BodyContext* ctx) {
    pred_visitor pv(pool, seq, var_map);
    std::vector<const expr::pred*> body;
    for (auto* p : ctx->pred())
        body.push_back(std::any_cast<const expr::pred*>(pv.visitPred(p)));
    return body;
}
