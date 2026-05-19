#include <stdexcept>
#include "../../hpp/infrastructure/normalizer.hpp"

normalizer::normalizer(i_expr_pool& expr_pool, i_bind_map& bind_map) :
    expr_pool_ref(expr_pool),
    bind_map_ref(bind_map) {
}

const expr* normalizer::normalize(const expr* e) {
    e = bind_map_ref.whnf(e);

    if (std::holds_alternative<expr::var>(e->content))
        return e;

    if (const expr::functor* f = std::get_if<expr::functor>(&e->content)) {
        std::vector<const expr*> normalized_args;
        normalized_args.reserve(f->args.size());
        for (const expr* arg : f->args)
            normalized_args.push_back(normalize(arg));
        return expr_pool_ref.functor(f->name, std::move(normalized_args));
    }

    throw std::runtime_error("Unsupported expression type");
}
