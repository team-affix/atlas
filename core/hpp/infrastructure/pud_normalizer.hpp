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
#include "value_objects/ubs_class.hpp"

// Rebuilds a framed term with every bound class resolved and every unbound variable named by its
// global representative. Each functor subterm is rebuilt once per call, so shared terms stay linear.
template<typename IGlobalize, typename IMakeFunctor, typename IMakeVar, typename IFind>
struct pud_normalizer {
    pud_normalizer(IGlobalize& globalize, IMakeFunctor& make_functor, IMakeVar& make_var, IFind& find);
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
};

#define PUD_NORMALIZER_TEMPLATE \
    template<typename IGlobalize, typename IMakeFunctor, typename IMakeVar, typename IFind>
#define PUD_NORMALIZER pud_normalizer<IGlobalize, IMakeFunctor, IMakeVar, IFind>

PUD_NORMALIZER_TEMPLATE
PUD_NORMALIZER::pud_normalizer(IGlobalize& globalize, IMakeFunctor& make_functor, IMakeVar& make_var, IFind& find)
    : globalize_(globalize)
    , make_functor_(make_functor)
    , make_var_(make_var)
    , find_(find) {}

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

// Returns the rebuilt term when it is already known, or pushes the functor still to be rebuilt.
PUD_NORMALIZER_TEMPLATE
std::optional<const expr*> PUD_NORMALIZER::enter(framed_expr framed, const memo_t& normalized, std::vector<frame>& pending) {
    if (const expr::var* var = std::get_if<expr::var>(&framed.skeleton->content)) {
        const ubs_class var_class = find_.find(globalize_.globalize(framed.frame_offset, var->index));
        if (!var_class.value.has_value())
            return make_var_.make_var(var_class.rep);
        framed = var_class.value.value();
    }
    if (const auto known = normalized.find(framed); known != normalized.end())
        return known->second;
    pending.push_back(frame{framed, {}});
    return std::nullopt;
}

#undef PUD_NORMALIZER
#undef PUD_NORMALIZER_TEMPLATE

#endif
