#include <stdexcept>
#include "infrastructure/normalizer.hpp"

normalizer::normalizer(locator& loc) :
    globalizer_ref(loc.locate<i_globalizer>()),
    make_functor_ref(loc.locate<i_make_functor>()),
    make_var_ref(loc.locate<i_make_var>()),
    bind_map_ref(loc.locate<i_bind_map>()) {
}

const expr* normalizer::normalize(framed_expr fe) {
    framed_expr resolved = bind_map_ref.whnf(fe);

    if (const expr::var* v = std::get_if<expr::var>(&resolved.skeleton->content))
        return make_var_ref.make_var(globalizer_ref.globalize(resolved.frame_offset, v->index));

    if (const expr::functor* f = std::get_if<expr::functor>(&resolved.skeleton->content)) {
        std::vector<const expr*> args;
        args.reserve(f->args.size());
        for (const expr* arg : f->args)
            args.push_back(normalize({arg, resolved.frame_offset}));
        return make_functor_ref.make_functor(f->id, args);
    }

    throw std::runtime_error("Unsupported expression type");
}
