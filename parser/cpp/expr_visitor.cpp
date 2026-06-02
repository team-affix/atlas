#include <any>
#include "../hpp/expr_visitor.hpp"

expr_visitor::expr_visitor(i_make_functor& make_functor, i_make_var& make_var, i_var_sequencer& var_seq,
                           std::map<std::string, uint32_t>& var_map)
    : make_functor(make_functor), make_var(make_var), var_seq(var_seq), var_map(var_map) {}

std::any expr_visitor::visitExpr(CHCParser::ExprContext* ctx) {
    if (auto* var = ctx->VARIABLE())
        return (const expr*)visitVar(var);

    return visitFunctor(ctx->functor());
}

const expr* expr_visitor::visitVar(antlr4::tree::TerminalNode* node) {
    const std::string& name = node->getText();
    if (name == "_")
        return make_var.make(var_seq.next());
    auto [it, inserted] = var_map.emplace(name, 0u);
    if (inserted) it->second = var_seq.next();
    return make_var.make(it->second);
}

std::any expr_visitor::visitFunctor(CHCParser::FunctorContext* ctx) {
    if (ctx->list())
        return visitList(ctx->list());

    auto* atom_token = ctx->ATOM();
    assert(atom_token != nullptr);
    std::string name = atom_token->getText();
    std::vector<const expr*> args;
    for (auto* sub_expr : ctx->expr())
        args.push_back(std::any_cast<const expr*>(visit(sub_expr)));
    return (const expr*)make_functor.make(name, std::move(args));
}

std::any expr_visitor::visitList(CHCParser::ListContext* ctx) {
    auto exprs = ctx->expr();

    if (exprs.empty())
        return (const expr*)make_functor.make("nil", {});

    bool has_pipe = false;
    for (auto* child : ctx->children) {
        if (child->getText() == "|") {
            has_pipe = true;
            break;
        }
    }

    size_t head_count = has_pipe ? exprs.size() - 1 : exprs.size();

    std::vector<const expr*> heads;
    heads.reserve(head_count);
    for (size_t i = 0; i < head_count; ++i)
        heads.push_back(std::any_cast<const expr*>(visit(exprs[i])));

    const expr* tail = has_pipe
        ? std::any_cast<const expr*>(visit(exprs.back()))
        : make_functor.make("nil", {});

    const expr* result = tail;
    for (int i = static_cast<int>(head_count) - 1; i >= 0; --i)
        result = make_functor.make("cons", {heads[i], result});
    return (const expr*)result;
}
