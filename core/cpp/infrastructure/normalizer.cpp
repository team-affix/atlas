#include <stdexcept>
#include "../../hpp/infrastructure/normalizer.hpp"
#include "../../hpp/bootstrap/resolver.hpp"

normalizer::normalizer() :
    expr_pool_ref(resolver::resolve<i_expr_pool>()),
    unifier_ref(resolver::resolve<i_unifier>()) {
}

const expr* normalizer::normalize(const expr* e) {
    e = unifier_ref.whnf(e);

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
