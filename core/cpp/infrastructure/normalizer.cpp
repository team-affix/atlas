#include <stdexcept>
#include "infrastructure/normalizer.hpp"

normalizer::normalizer(locator& loc) :
    make_functor_ref(loc.locate<i_make_functor>()),
    bind_map_ref(loc.locate<i_bind_map>()) {
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
        return make_functor_ref.make(f->name, normalized_args);
    }

    throw std::runtime_error("Unsupported expression type");
}
