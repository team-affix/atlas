#ifndef PUD_UNIFIER_HPP
#define PUD_UNIFIER_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <variant>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"

template<
    typename IGlobalize,
    typename IFind,
    typename IFindValue,
    typename IEquate,
    typename IInstantiate,
    typename IDetectCycle>
struct pud_unifier {
    pud_unifier(
        IGlobalize& globalize,
        IFind& find,
        IFindValue& find_value,
        IEquate& equate,
        IInstantiate& instantiate,
        IDetectCycle& detect_cycle);
    bool unify(framed_expr lhs, framed_expr rhs);
private:
    uint32_t find_root(framed_expr var);

    IGlobalize& globalize_;
    IFind& find_;
    IFindValue& find_value_;
    IEquate& equate_;
    IInstantiate& instantiate_;
    IDetectCycle& detect_cycle_;
};

#define PUD_UNIFIER_TEMPLATE                                                                     \
    template<typename IGlobalize, typename IFind, typename IFindValue, typename IEquate,        \
             typename IInstantiate, typename IDetectCycle>
#define PUD_UNIFIER pud_unifier<IGlobalize, IFind, IFindValue, IEquate, IInstantiate, IDetectCycle>

PUD_UNIFIER_TEMPLATE
PUD_UNIFIER::pud_unifier(
    IGlobalize& globalize,
    IFind& find,
    IFindValue& find_value,
    IEquate& equate,
    IInstantiate& instantiate,
    IDetectCycle& detect_cycle)
    : globalize_(globalize)
    , find_(find)
    , find_value_(find_value)
    , equate_(equate)
    , instantiate_(instantiate)
    , detect_cycle_(detect_cycle) {}

PUD_UNIFIER_TEMPLATE
bool PUD_UNIFIER::unify(framed_expr lhs, framed_expr rhs) {
    std::vector<std::pair<framed_expr, framed_expr>> pending{{lhs, rhs}};
    std::vector<uint32_t> touched;
    while (!pending.empty()) {
        const auto [left, right] = pending.back();
        pending.pop_back();
        const bool left_is_var = std::holds_alternative<expr::var>(left.skeleton->content);
        const bool right_is_var = std::holds_alternative<expr::var>(right.skeleton->content);

        if (left_is_var && right_is_var) {
            const uint32_t left_root = find_root(left);
            const uint32_t right_root = find_root(right);
            if (left_root == right_root)
                continue;
            const std::optional<framed_expr> left_value = find_value_.find_value(left_root);
            const std::optional<framed_expr> right_value = find_value_.find_value(right_root);
            equate_.equate(left_root, right_root);
            touched.push_back(left_root);
            if (left_value.has_value() && right_value.has_value())
                pending.emplace_back(left_value.value(), right_value.value());
            continue;
        }

        if (left_is_var || right_is_var) {
            const framed_expr var = left_is_var ? left : right;
            const framed_expr term = left_is_var ? right : left;
            const uint32_t root = find_root(var);
            if (const std::optional<framed_expr> value = find_value_.find_value(root)) {
                pending.emplace_back(value.value(), term);
                continue;
            }
            instantiate_.instantiate(root, term);
            touched.push_back(root);
            continue;
        }

        const expr::functor& left_functor = std::get<expr::functor>(left.skeleton->content);
        const expr::functor& right_functor = std::get<expr::functor>(right.skeleton->content);
        if (left_functor.id != right_functor.id || left_functor.args.size() != right_functor.args.size())
            return false;
        for (size_t arg = left_functor.args.size(); arg-- > 0;)
            pending.emplace_back(
                framed_expr{left_functor.args[arg], left.frame_offset},
                framed_expr{right_functor.args[arg], right.frame_offset});
    }
    return !detect_cycle_.detect_cycle(touched);
}

PUD_UNIFIER_TEMPLATE
uint32_t PUD_UNIFIER::find_root(framed_expr var) {
    const uint32_t index = std::get<expr::var>(var.skeleton->content).index;
    return find_.find(globalize_.globalize(var.frame_offset, index));
}

#undef PUD_UNIFIER
#undef PUD_UNIFIER_TEMPLATE

#endif
