#ifndef NORMALIZER_HPP
#define NORMALIZER_HPP

#include <cstdint>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include "value_objects/framed_expr.hpp"
#include "value_objects/expr.hpp"

template<typename IGlobalize, typename IMakeFunctor, typename IMakeVar, typename IWhnf>
struct normalizer {
    normalizer(IGlobalize&, IMakeFunctor&, IMakeVar&, IWhnf&);
    const expr* normalize(framed_expr, uint32_t cutoff,
                          std::unordered_map<uint32_t, uint32_t>& translation);
private:
    IGlobalize& globalizer_ref_;
    IMakeFunctor& make_functor_ref_;
    IMakeVar& make_var_ref_;
    IWhnf& bind_map_ref_;
};

template<typename IG, typename IMF, typename IMV, typename IW>
normalizer<IG,IMF,IMV,IW>::normalizer(IG& g, IMF& mf, IMV& mv, IW& bm)
    : globalizer_ref_(g), make_functor_ref_(mf), make_var_ref_(mv), bind_map_ref_(bm) {}

template<typename IG, typename IMF, typename IMV, typename IW>
const expr* normalizer<IG,IMF,IMV,IW>::normalize(
    framed_expr fe,
    uint32_t cutoff,
    std::unordered_map<uint32_t, uint32_t>& translation) {
    const expr::var* incoming_var = std::get_if<expr::var>(&fe.skeleton->content);
    const bool frozen_incoming_var = incoming_var
        && globalizer_ref_.globalize(fe.frame_offset, incoming_var->index) < cutoff;
    if (!frozen_incoming_var)
        fe = bind_map_ref_.whnf(fe);

    if (const expr::var* v = std::get_if<expr::var>(&fe.skeleton->content)) {
        const uint32_t key = globalizer_ref_.globalize(fe.frame_offset, v->index);
        const bool frozen = key < cutoff;
        if (frozen)
            return make_var_ref_.make_var(key);
        if (translation.contains(key))
            return make_var_ref_.make_var(translation.at(key));
        const uint32_t renamed = cutoff + static_cast<uint32_t>(translation.size());
        translation.emplace(key, renamed);
        return make_var_ref_.make_var(renamed);
    }

    if (const expr::functor* f = std::get_if<expr::functor>(&fe.skeleton->content)) {
        std::vector<const expr*> args;
        args.reserve(f->args.size());
        for (const expr* arg : f->args)
            args.push_back(normalize({arg, fe.frame_offset}, cutoff, translation));
        return make_functor_ref_.make_functor(f->id, args);
    }

    throw std::runtime_error("Unsupported expression type");
}

#endif
