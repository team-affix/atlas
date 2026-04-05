#include <stdexcept>
#include "../hpp/expr.hpp"

expr_pool::expr_pool(trail& t, sequencer& seq, registry<expr>& er) :
    trail_ref(t), seq(seq), er(er) {

}

expr_id expr_pool::atom(const std::string& s) {
    return intern(expr{expr::atom{s}});
}

expr_id expr_pool::var(uint32_t i) {
    return intern(expr{expr::var{i}});
}

expr_id expr_pool::cons(expr_id l, expr_id r) {
    return intern(expr{expr::cons{l, r}});
}

expr_id expr_pool::import(expr_id e) {
    // if the expression is a leaf, just intern it
    if (std::holds_alternative<expr::atom>(er.deref(e)->content) ||
        std::holds_alternative<expr::var>(er.deref(e)->content))
        return intern(expr{er.deref(e)->content});
    
    // if the expression is a cons cell, copy the lhs and rhs
    if (const expr::cons* c = std::get_if<expr::cons>(&er.deref(e)->content))
        return cons(import(c->lhs), import(c->rhs));

    throw std::runtime_error("Unsupported expression type");
}

size_t expr_pool::size() const {
    return exprs.size();
}

expr_id expr_pool::intern(expr&& e) {
    auto [it, inserted] = exprs.insert(std::move(e));

    if (!inserted)
        return er.ref(&*it);

    // Generate a new id for the expression
    expr_id id = seq();

    // Insert the expression into the registry
    er.insert(id, &*it);
    
    trail_ref.log(
        [this, val = *it, id]() { exprs.erase(val); er.erase(id); },
        [this, val = *it, id]() { auto [it, _] = exprs.insert(val); er.insert(id, &*it); }
    );
    
    return id;
}
