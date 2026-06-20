#ifndef NORMALIZER_HPP
#define NORMALIZER_HPP

#include <stdexcept>
#include <vector>
#include "value_objects/framed_expr.hpp"
#include "value_objects/expr.hpp"

template<typename IGlobalize, typename IMakeFunctor, typename IMakeVar, typename IWhnf>
struct normalizer {
    normalizer(IGlobalize&, IMakeFunctor&, IMakeVar&, IWhnf&);
    const expr* normalize(framed_expr);
private:
    IGlobalize& globalizer_ref;
    IMakeFunctor& make_functor_ref;
    IMakeVar& make_var_ref;
    IWhnf& bind_map_ref;
};

template<typename IG, typename IMF, typename IMV, typename IW>
normalizer<IG,IMF,IMV,IW>::normalizer(IG& g, IMF& mf, IMV& mv, IW& bm)
    : globalizer_ref(g), make_functor_ref(mf), make_var_ref(mv), bind_map_ref(bm) {}

template<typename IG, typename IMF, typename IMV, typename IW>
const expr* normalizer<IG,IMF,IMV,IW>::normalize(framed_expr fe) {
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

#endif
