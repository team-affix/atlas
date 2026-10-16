#ifndef GLOBAL_NORMALIZER_HPP
#define GLOBAL_NORMALIZER_HPP

#include <variant>
#include <vector>
#include "value_objects/framed_expr.hpp"
#include "value_objects/expr.hpp"

template<typename IGlobalize, typename IMakeFunctor, typename IResolve>
struct global_normalizer {
    global_normalizer(IGlobalize& globalize, IMakeFunctor& make_functor, IResolve& resolve);
    const expr* normalize(framed_expr fe);

private:
    IGlobalize& globalize_;
    IMakeFunctor& make_functor_;
    IResolve& resolve_;
};

template<typename IGlobalize, typename IMakeFunctor, typename IResolve>
global_normalizer<IGlobalize, IMakeFunctor, IResolve>::global_normalizer(
    IGlobalize& globalize, IMakeFunctor& make_functor, IResolve& resolve)
    : globalize_(globalize)
    , make_functor_(make_functor)
    , resolve_(resolve) {}

template<typename IGlobalize, typename IMakeFunctor, typename IResolve>
const expr* global_normalizer<IGlobalize, IMakeFunctor, IResolve>::normalize(framed_expr fe) {
    if (const expr::var* var = std::get_if<expr::var>(&fe.skeleton->content)) {
        const expr* value = resolve_.resolve(globalize_.globalize(fe.frame_offset, var->index));
        if (std::holds_alternative<expr::var>(value->content))
            return value;
        fe = {value, 0u};
    }

    const expr::functor& functor = std::get<expr::functor>(fe.skeleton->content);
    std::vector<const expr*> args;
    for (const expr* arg : functor.args)
        args.push_back(normalize({arg, fe.frame_offset}));
    return make_functor_.make_functor(functor.id, args);
}

#endif
