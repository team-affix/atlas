#include <stdexcept>
#include "infrastructure/globalizer.hpp"

globalizer::globalizer(locator& loc) :
    make_functor_ref(loc.locate<i_make_functor>()),
    make_var_ref(loc.locate<i_make_var>()) {
}

const expr* globalizer::globalize(framed_expr fe) {
    const expr* e = fe.skeleton;
    uint32_t frame_offset = fe.frame_offset;

    if (const expr::var* v = std::get_if<expr::var>(&e->content))
        return make_var_ref.make_var(frame_offset + v->index);

    if (const expr::functor* f = std::get_if<expr::functor>(&e->content)) {
        std::vector<const expr*> globalized_args;
        globalized_args.reserve(f->args.size());
        for (const expr* arg : f->args)
            globalized_args.push_back(globalize({arg, frame_offset}));
        return make_functor_ref.make_functor(f->id, globalized_args);
    }

    throw std::runtime_error("Unsupported expression type");
}
