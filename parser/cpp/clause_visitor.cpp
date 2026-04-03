#include "../hpp/clause_visitor.hpp"

clause_visitor::clause_visitor(expr_pool& pool, sequencer& seq)
    : pool(pool), seq(seq) {}

antlrcpp::Any clause_visitor::visitClause(CHCParser::ClauseContext* ctx) {
    std::map<std::string, uint32_t> var_map;
    expr_visitor ev(pool, seq, var_map);

    const expr* head = std::any_cast<const expr*>(ev.visitExpr(ctx->expr()));

    auto* b = ctx->body();
    if (!b) return rule{head, {}};

    body_visitor bv(pool, seq, var_map);
    auto body = std::any_cast<std::vector<const expr*>>(bv.visitBody(b));
    return rule{head, body};
}
