#ifndef PUD_TRANSLATING_NORMALIZER_HPP
#define PUD_TRANSLATING_NORMALIZER_HPP

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"

template<typename IGlobalize, typename IMakeFunctor, typename IMakeVar, typename IFindRep, typename IFindValue>
struct pud_translating_normalizer {
    pud_translating_normalizer(
        IGlobalize& globalize,
        IMakeFunctor& make_functor,
        IMakeVar& make_var,
        IFindRep& find_rep,
        IFindValue& find_value);
    const expr* normalize(
        framed_expr framed,
        uint32_t fresh_offset,
        std::unordered_map<uint32_t, uint32_t>& translation);
private:
    struct frame {
        framed_expr functor;
        std::vector<const expr*> args;
    };

    const expr* enter(
        framed_expr framed,
        uint32_t fresh_offset,
        std::unordered_map<uint32_t, uint32_t>& translation,
        std::vector<frame>& pending);

    IGlobalize& globalize_;
    IMakeFunctor& make_functor_;
    IMakeVar& make_var_;
    IFindRep& find_rep_;
    IFindValue& find_value_;
};

#define PUD_TRANSLATING_NORMALIZER_TEMPLATE                                                \
    template<typename IGlobalize, typename IMakeFunctor, typename IMakeVar, typename IFindRep,       \
             typename IFindValue>
#define PUD_TRANSLATING_NORMALIZER                                                         \
    pud_translating_normalizer<IGlobalize, IMakeFunctor, IMakeVar, IFindRep, IFindValue>

PUD_TRANSLATING_NORMALIZER_TEMPLATE
PUD_TRANSLATING_NORMALIZER::pud_translating_normalizer(
    IGlobalize& globalize,
    IMakeFunctor& make_functor,
    IMakeVar& make_var,
    IFindRep& find_rep,
    IFindValue& find_value)
    : globalize_(globalize)
    , make_functor_(make_functor)
    , make_var_(make_var)
    , find_rep_(find_rep)
    , find_value_(find_value) {}

PUD_TRANSLATING_NORMALIZER_TEMPLATE
const expr* PUD_TRANSLATING_NORMALIZER::normalize(
    framed_expr framed,
    uint32_t fresh_offset,
    std::unordered_map<uint32_t, uint32_t>& translation) {
    std::vector<frame> pending;
    const expr* finished = enter(framed, fresh_offset, translation, pending);
    while (!pending.empty()) {
        frame& top = pending.back();
        if (finished) {
            top.args.push_back(finished);
            finished = nullptr;
        }
        const expr::functor& functor = std::get<expr::functor>(top.functor.skeleton->content);
        if (top.args.size() == functor.args.size()) {
            finished = make_functor_.make_functor(functor.id, top.args);
            pending.pop_back();
            continue;
        }
        const framed_expr next_arg{functor.args.at(top.args.size()), top.functor.frame_offset};
        finished = enter(next_arg, fresh_offset, translation, pending);
    }
    return finished;
}

PUD_TRANSLATING_NORMALIZER_TEMPLATE
const expr* PUD_TRANSLATING_NORMALIZER::enter(
    framed_expr framed,
    uint32_t fresh_offset,
    std::unordered_map<uint32_t, uint32_t>& translation,
    std::vector<frame>& pending) {
    if (const expr::var* var = std::get_if<expr::var>(&framed.skeleton->content)) {
        const uint32_t global = globalize_.globalize(framed.frame_offset, var->index);
        const std::optional<framed_expr> value = find_value_.find_value(global);
        if (!value.has_value()) {
            const uint32_t rep = find_rep_.find_rep(global);
            if (rep < fresh_offset)
                return make_var_.make_var(rep);
            auto [translated, _] = translation.try_emplace(
                rep,
                fresh_offset + static_cast<uint32_t>(translation.size()));
            return make_var_.make_var(translated->second);
        }
        framed = value.value();
    }
    pending.push_back(frame{framed, {}});
    return nullptr;
}

#undef PUD_TRANSLATING_NORMALIZER
#undef PUD_TRANSLATING_NORMALIZER_TEMPLATE

#endif
