#include "../hpp/pred_visitor.hpp"

pred_visitor::pred_visitor(expr_pool& pool, sequencer& seq, std::map<std::string, uint32_t>& var_map)
    : pool(pool), seq(seq), var_map(var_map) {}

antlrcpp::Any pred_visitor::visitPred(CHCParser::PredContext* ctx) {
    // Functor name is always the leading ATOM token.
    std::string name = ctx->ATOM()->getText();

    // Build the args vector from the expr children.
    expr_visitor ev(pool, seq, var_map);
    std::vector<const expr*> args;
    for (auto* e : ctx->expr())
        args.push_back(std::any_cast<const expr*>(ev.visitExpr(e)));

    return &std::get<expr::pred>(pool.pred(name, std::move(args))->content);
}
