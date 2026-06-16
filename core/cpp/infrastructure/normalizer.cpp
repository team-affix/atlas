#include <stdexcept>
#include "infrastructure/normalizer.hpp"

normalizer::normalizer(locator& loc) :
    globalizer_ref(loc.locate<i_globalizer>()),
    make_functor_ref(loc.locate<i_make_functor>()),
    bind_map_ref(loc.locate<i_bind_map>()) {
}

const expr* normalizer::normalize(const expr* e) {
    if (const expr::var* v = std::get_if<expr::var>(&e->content)) {
        framed_expr resolved = bind_map_ref.whnf({e, 0});
        const expr* globalized = globalizer_ref.globalize(resolved);
        if (std::get_if<expr::var>(&resolved.skeleton->content))
            return globalized;
        return normalize(globalized);
    }

    if (const expr::functor* f = std::get_if<expr::functor>(&e->content)) {
        std::vector<const expr*> args;
        args.reserve(f->args.size());
        for (const expr* arg : f->args)
            args.push_back(normalize(arg));
        return make_functor_ref.make_functor(f->id, args);
    }

    throw std::runtime_error("Unsupported expression type");
}
