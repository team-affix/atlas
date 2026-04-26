#include <stdexcept>
#include "../hpp/normalizer.hpp"
#include "../hpp/locator.hpp"

normalizer::normalizer() :
    expr_pool_ref(locator::locate<expr_pool>(locator_keys::inst_expr_pool)),
    bind_map_ref(locator::locate<bind_map>(locator_keys::inst_bind_map)) {
}

const expr* normalizer::operator()(const expr* e) {
    // First, get the whnf
    e = bind_map_ref.whnf(e);
    
    // If the expression is a variable, return it unchanged since it is already normalized
    if (std::holds_alternative<expr::var>(e->content))
        return e;

    // If the expression is a functor, normalize all args recursively
    if (const expr::functor* f = std::get_if<expr::functor>(&e->content)) {
        std::vector<const expr*> normalized_args;
        normalized_args.reserve(f->args.size());
        for (const expr* arg : f->args)
            normalized_args.push_back(operator()(arg));
        return expr_pool_ref.functor(f->name, std::move(normalized_args));
    }

    throw std::runtime_error("Unsupported expression type");
    
}
