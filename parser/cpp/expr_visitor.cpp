#include <any>
#include "../hpp/expr_visitor.hpp"

expr_visitor::expr_visitor(expr_pool& pool, sequencer& seq, std::map<std::string, uint32_t>& var_map)
    : pool(pool), seq(seq), var_map(var_map) {}

std::any expr_visitor::visitExpr(CHCParser::ExprContext* ctx) {
    if (auto* var = ctx->VARIABLE())
        return (const expr*)visitVar(var);

    return visitFunctor(ctx->functor());
}

const expr* expr_visitor::visitVar(antlr4::tree::TerminalNode* node) {
    const std::string& name = node->getText();
    if (name == "_")
        return pool.var(seq());
    auto [it, inserted] = var_map.emplace(name, 0u);
    if (inserted) it->second = seq();
    return pool.var(it->second);
}

std::any expr_visitor::visitFunctor(CHCParser::FunctorContext* ctx) {
    if (ctx->list())
        return visitList(ctx->list());

    // ATOM with optional parenthesised arg list
    auto* atom_token = ctx->ATOM();
    assert(atom_token != nullptr);
    std::string name = atom_token->getText();
    std::vector<const expr*> args;
    for (auto* sub_expr : ctx->expr())
        args.push_back(std::any_cast<const expr*>(visit(sub_expr)));
    return (const expr*)pool.functor(name, std::move(args));
}

std::any expr_visitor::visitList(CHCParser::ListContext* ctx) {
    auto exprs = ctx->expr();

    // '[]' → nil
    if (exprs.empty())
        return (const expr*)pool.functor("nil", {});

    // Check whether there is a '|' token among the children
    bool has_pipe = false;
    for (auto* child : ctx->children) {
        if (child->getText() == "|") {
            has_pipe = true;
            break;
        }
    }

    // Collect head elements (all exprs except last if pipe present)
    size_t head_count = has_pipe ? exprs.size() - 1 : exprs.size();

    // Evaluate head elements
    std::vector<const expr*> heads;
    heads.reserve(head_count);
    for (size_t i = 0; i < head_count; ++i)
        heads.push_back(std::any_cast<const expr*>(visit(exprs[i])));

    // Tail: explicit if pipe present, else nil
    const expr* tail = has_pipe
        ? std::any_cast<const expr*>(visit(exprs.back()))
        : pool.functor("nil", {});

    // Right-fold: cons(h1, cons(h2, ..., tail))
    const expr* result = tail;
    for (int i = static_cast<int>(head_count) - 1; i >= 0; --i)
        result = pool.functor("cons", {heads[i], result});
    return (const expr*)result;
}
