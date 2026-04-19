#include <stdexcept>
#include "../hpp/normalizer.hpp"

normalizer::normalizer(expr_pool& expr_pool_ref, bind_map& bind_map_ref)
    : expr_pool_ref(expr_pool_ref), bind_map_ref(bind_map_ref) {

}

const expr* normalizer::operator()(const expr* e) {
    // First, get the whnf
    e = bind_map_ref.whnf(e);
    
    // If the expression is an atom, return it unchanged
    if (std::holds_alternative<expr::atom>(e->content))
        return e;
    
    // If the expression is a variable, return it unchanged since it is already normalized
    if (std::holds_alternative<expr::var>(e->content))
        return e;

    // If the expression is a cons cell, normalize the lhs and rhs
    if (const expr::cons* c = std::get_if<expr::cons>(&e->content)) {
        const expr* normalized_lhs = operator()(c->lhs);
        const expr* normalized_rhs = operator()(c->rhs);
        return expr_pool_ref.cons(normalized_lhs, normalized_rhs);
    }

    // If the expression is a pred, normalize each argument
    if (const expr::pred* p = std::get_if<expr::pred>(&e->content)) {
        std::vector<const expr*> normalized_args;
        normalized_args.reserve(p->args.size());
        for (const expr* arg : p->args)
            normalized_args.push_back(operator()(arg));
        return expr_pool_ref.pred(p->name, std::move(normalized_args));
    }

    throw std::runtime_error("Unsupported expression type");
    
}
