#include <stdexcept>
#include "../hpp/expr.hpp"

expr_pool::expr_pool(trail& t) : trail_ref(t) {

}

const expr* expr_pool::atom(const std::string& s) {
    return intern(expr{expr::atom{s}});
}

const expr* expr_pool::var(uint32_t i) {
    return intern(expr{expr::var{i}});
}

const expr* expr_pool::cons(const expr* l, const expr* r) {
    return intern(expr{expr::cons{l, r}});
}

const expr* expr_pool::pred(const std::string& name, std::vector<const expr*> args) {
    return intern(expr{expr::pred{name, std::move(args)}});
}

const expr* expr_pool::as_expr(const expr::pred* p) {
    // Re-intern the pred to get back the stable const expr* container.
    // The args are already interned pointers so this lookup is idempotent.
    std::vector<const expr*> args(p->args.begin(), p->args.end());
    return pred(p->name, std::move(args));
}

const expr* expr_pool::import(const expr* e) {
    // if the expression is a leaf, just intern it
    if (std::holds_alternative<expr::atom>(e->content) ||
        std::holds_alternative<expr::var>(e->content))
        return intern(expr{e->content});
    
    // if the expression is a cons cell, copy the lhs and rhs
    if (const expr::cons* c = std::get_if<expr::cons>(&e->content))
        return cons(import(c->lhs), import(c->rhs));

    // if the expression is a pred, import each argument
    if (const expr::pred* p = std::get_if<expr::pred>(&e->content)) {
        std::vector<const expr*> imported_args;
        imported_args.reserve(p->args.size());
        for (const expr* arg : p->args)
            imported_args.push_back(import(arg));
        return intern(expr{expr::pred{p->name, std::move(imported_args)}});
    }

    throw std::runtime_error("Unsupported expression type");
}

size_t expr_pool::size() const {
    return exprs.size();
}

const expr* expr_pool::intern(expr&& e) {
    auto [it, inserted] = exprs.insert(std::move(e));
    if (inserted) trail_ref.log([this, it]() { exprs.erase(it); });
    return &*it;
}
