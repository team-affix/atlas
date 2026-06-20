#ifndef NORMALIZER_HPP
#define NORMALIZER_HPP

#include <stdexcept>
#include <vector>
#include "value_objects/framed_expr.hpp"
#include "value_objects/expr.hpp"

template<typename IGlobalizer, typename IExprPool, typename IBindMap>
struct normalizer {
    normalizer(IGlobalizer&, IExprPool&, IBindMap&);
    const expr* normalize(framed_expr);
private:
    IGlobalizer& globalizer_ref;
    IExprPool& expr_pool_ref;
    IBindMap& bind_map_ref;
};

template<typename IG, typename IEP, typename IBM>
normalizer<IG,IEP,IBM>::normalizer(IG& g, IEP& ep, IBM& bm)
    : globalizer_ref(g), expr_pool_ref(ep), bind_map_ref(bm) {}

template<typename IG, typename IEP, typename IBM>
const expr* normalizer<IG,IEP,IBM>::normalize(framed_expr fe) {
    framed_expr resolved = bind_map_ref.whnf(fe);
    if (const expr::var* v = std::get_if<expr::var>(&resolved.skeleton->content))
        return expr_pool_ref.make_var(globalizer_ref.globalize(resolved.frame_offset, v->index));
    if (const expr::functor* f = std::get_if<expr::functor>(&resolved.skeleton->content)) {
        std::vector<const expr*> args;
        args.reserve(f->args.size());
        for (const expr* arg : f->args)
            args.push_back(normalize({arg, resolved.frame_offset}));
        return expr_pool_ref.make_functor(f->id, args);
    }
    throw std::runtime_error("Unsupported expression type");
}

#endif
