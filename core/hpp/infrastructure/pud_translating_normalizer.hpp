#ifndef PUD_TRANSLATING_NORMALIZER_HPP
#define PUD_TRANSLATING_NORMALIZER_HPP

#include <cstdint>
#include <unordered_map>
#include <variant>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"

// Fully normalizes a framed expression; representatives at or above fresh_offset are renamed
// densely from fresh_offset in the order they are first met, shared through translation.
template<typename IGlobalize, typename IMakeFunctor, typename IMakeVar, typename IResolve>
struct pud_translating_normalizer {
    pud_translating_normalizer(
        IGlobalize& globalize,
        IMakeFunctor& make_functor,
        IMakeVar& make_var,
        IResolve& resolve);
    const expr* normalize(
        framed_expr framed,
        uint32_t fresh_offset,
        std::unordered_map<uint32_t, uint32_t>& translation);
private:
    IGlobalize& globalize_;
    IMakeFunctor& make_functor_;
    IMakeVar& make_var_;
    IResolve& resolve_;
};

#define PUD_TRANSLATING_NORMALIZER_TEMPLATE                                                \
    template<typename IGlobalize, typename IMakeFunctor, typename IMakeVar, typename IResolve>
#define PUD_TRANSLATING_NORMALIZER                                                         \
    pud_translating_normalizer<IGlobalize, IMakeFunctor, IMakeVar, IResolve>

PUD_TRANSLATING_NORMALIZER_TEMPLATE
PUD_TRANSLATING_NORMALIZER::pud_translating_normalizer(
    IGlobalize& globalize,
    IMakeFunctor& make_functor,
    IMakeVar& make_var,
    IResolve& resolve)
    : globalize_(globalize)
    , make_functor_(make_functor)
    , make_var_(make_var)
    , resolve_(resolve) {}

PUD_TRANSLATING_NORMALIZER_TEMPLATE
const expr* PUD_TRANSLATING_NORMALIZER::normalize(
    framed_expr framed,
    uint32_t fresh_offset,
    std::unordered_map<uint32_t, uint32_t>& translation) {
    if (const expr::var* var = std::get_if<expr::var>(&framed.skeleton->content)) {
        const expr* value = resolve_.resolve(globalize_.globalize(framed.frame_offset, var->index));
        if (const expr::var* rep = std::get_if<expr::var>(&value->content)) {
            if (rep->index < fresh_offset)
                return value;
            auto [translated, _] = translation.try_emplace(
                rep->index,
                fresh_offset + static_cast<uint32_t>(translation.size()));
            return make_var_.make_var(translated->second);
        }
        framed = framed_expr{value, 0};
    }
    const expr::functor& functor = std::get<expr::functor>(framed.skeleton->content);
    std::vector<const expr*> args;
    args.reserve(functor.args.size());
    for (const expr* arg : functor.args)
        args.push_back(normalize(framed_expr{arg, framed.frame_offset}, fresh_offset, translation));
    return make_functor_.make_functor(functor.id, args);
}

#undef PUD_TRANSLATING_NORMALIZER
#undef PUD_TRANSLATING_NORMALIZER_TEMPLATE

#endif
