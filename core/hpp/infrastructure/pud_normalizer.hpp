#ifndef PUD_NORMALIZER_HPP
#define PUD_NORMALIZER_HPP

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/framed_expr_hash.hpp"
#include "value_objects/moduled_functor.hpp"
#include "value_objects/ubs_class.hpp"

template<typename IGlobalize, typename IMakeFunctor, typename IMakeVar, typename IFind, typename IDemoduleFunctor>
struct pud_normalizer {
    pud_normalizer(
        IGlobalize& globalize,
        IMakeFunctor& make_functor,
        IMakeVar& make_var,
        IFind& find,
        IDemoduleFunctor& demodule_functor);
    const expr* normalize(framed_expr framed);
private:
    using memo_t = std::unordered_map<framed_expr, const expr*, framed_expr_hash>;
    struct frame {
        framed_expr functor;
        std::vector<const expr*> args;
    };

    std::optional<const expr*> enter(framed_expr framed, const memo_t& normalized, std::vector<frame>& pending);

    IGlobalize& globalize_;
    IMakeFunctor& make_functor_;
    IMakeVar& make_var_;
    IFind& find_;
    IDemoduleFunctor& demodule_functor_;
};

#define PUD_NORMALIZER_TEMPLATE \
    template<typename IGlobalize, typename IMakeFunctor, typename IMakeVar, typename IFind, typename IDemoduleFunctor>
#define PUD_NORMALIZER pud_normalizer<IGlobalize, IMakeFunctor, IMakeVar, IFind, IDemoduleFunctor>

PUD_NORMALIZER_TEMPLATE
PUD_NORMALIZER::pud_normalizer(
    IGlobalize& globalize,
    IMakeFunctor& make_functor,
    IMakeVar& make_var,
    IFind& find,
    IDemoduleFunctor& demodule_functor)
    : globalize_(globalize)
    , make_functor_(make_functor)
    , make_var_(make_var)
    , find_(find)
    , demodule_functor_(demodule_functor) {}

PUD_NORMALIZER_TEMPLATE
const expr* PUD_NORMALIZER::normalize(framed_expr framed) {
    memo_t normalized;
    std::vector<frame> pending;
    std::optional<const expr*> finished = enter(framed, normalized, pending);
    while (!pending.empty()) {
        frame& top = pending.back();
        if (finished.has_value()) {
            top.args.push_back(finished.value());
            finished = std::nullopt;
        }
        const expr::functor& functor = std::get<expr::functor>(top.functor.skeleton->content);
        if (top.args.size() == functor.args.size()) {
            finished = make_functor_.make_functor(functor.id, top.args);
            normalized.emplace(top.functor, finished.value());
            pending.pop_back();
            continue;
        }
        finished = enter(framed_expr{functor.args.at(top.args.size()), top.functor.frame_offset}, normalized, pending);
    }
    return finished.value();
}

PUD_NORMALIZER_TEMPLATE
std::optional<const expr*> PUD_NORMALIZER::enter(framed_expr framed, const memo_t& normalized, std::vector<frame>& pending) {
    if (const expr::var* var = std::get_if<expr::var>(&framed.skeleton->content)) {
        const ubs_class var_class = find_.find(globalize_.globalize(framed.frame_offset, var->index));
        if (!var_class.value.has_value())
            return make_var_.make_var(var_class.rep);
        const expr::functor& functor = *var_class.value->functor;
        const expr* reconstructed = demodule_functor_.demodule(
            moduled_functor{&functor, var_class.value->frame_offset, var_class.value->module});
        framed = framed_expr{reconstructed, var_class.value->frame_offset};
    }
    if (const auto known = normalized.find(framed); known != normalized.end())
        return known->second;
    pending.push_back(frame{framed, {}});
    return std::nullopt;
}

#undef PUD_NORMALIZER
#undef PUD_NORMALIZER_TEMPLATE

#endif
