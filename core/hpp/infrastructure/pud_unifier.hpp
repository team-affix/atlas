#ifndef PUD_UNIFIER_HPP
#define PUD_UNIFIER_HPP

#include <cstddef>
#include <cstdint>
#include <utility>
#include <variant>
#include <vector>
#include "value_objects/framed_expr.hpp"
#include "value_objects/expr.hpp"

// Unifies framed terms. Before binding a variable's class to a functor, the
// functor is normalized and checked for the variable (occurs check). Iterative, so
// term depth is bounded by memory rather than by the call stack.
template<
    typename IGlobalize,
    typename INormalize,
    typename IResolve,
    typename IEquate,
    typename IInstantiate>
struct pud_unifier {
    pud_unifier(
        IGlobalize& globalize,
        INormalize& normalize,
        IResolve& resolve,
        IEquate& equate,
        IInstantiate& instantiate);
    bool unify(framed_expr lhs, framed_expr rhs);

private:
    // Whether a variable with this global index occurs in the normalized term.
    bool occurs(uint32_t var_index, const expr* normalized);

    IGlobalize& globalize_;
    INormalize& normalize_;
    IResolve& resolve_;
    IEquate& equate_;
    IInstantiate& instantiate_;
};

#define PUD_UNIFIER_TEMPLATE                                                     \
    template<typename IGlobalize, typename INormalize, typename IResolve,           \
             typename IEquate, typename IInstantiate>
#define PUD_UNIFIER pud_unifier<IGlobalize, INormalize, IResolve, IEquate, IInstantiate>

PUD_UNIFIER_TEMPLATE
PUD_UNIFIER::pud_unifier(
    IGlobalize& globalize,
    INormalize& normalize,
    IResolve& resolve,
    IEquate& equate,
    IInstantiate& instantiate)
    : globalize_(globalize)
    , normalize_(normalize)
    , resolve_(resolve)
    , equate_(equate)
    , instantiate_(instantiate) {}

PUD_UNIFIER_TEMPLATE
bool PUD_UNIFIER::unify(framed_expr lhs, framed_expr rhs) {
    std::vector<std::pair<framed_expr, framed_expr>> pending{{lhs, rhs}};
    while (!pending.empty()) {
        auto [left, right] = pending.back();
        pending.pop_back();

        // Replace each variable with its class's value: a functor, or the rep var.
        // Values are in global indices, so they carry offset 0.
        if (const expr::var* var = std::get_if<expr::var>(&left.skeleton->content))
            left = {resolve_.resolve(globalize_.globalize(left.frame_offset, var->index)), 0u};
        if (const expr::var* var = std::get_if<expr::var>(&right.skeleton->content))
            right = {resolve_.resolve(globalize_.globalize(right.frame_offset, var->index)), 0u};

        const expr::var* left_var = std::get_if<expr::var>(&left.skeleton->content);
        const expr::var* right_var = std::get_if<expr::var>(&right.skeleton->content);

        const uint32_t left_global = left_var ? globalize_.globalize(left.frame_offset, left_var->index) : 0;
        const uint32_t right_global = right_var ? globalize_.globalize(right.frame_offset, right_var->index) : 0;

        if (left_var && right_var && left_global == right_global)
            continue;

        if (left_var && right_var) {
            equate_.equate(left_var->index, right_var->index);
            continue;
        }
        if (left_var) {
            const expr* value = normalize_.normalize(right);
            if (occurs(left_var->index, value))
                return false;
            const expr::functor& bound_functor = std::get<expr::functor>(value->content);
            instantiate_.instantiate(left_var->index, bound_functor.id, bound_functor.args);
            continue;
        }
        if (right_var) {
            const expr* value = normalize_.normalize(left);
            if (occurs(right_var->index, value))
                return false;
            const expr::functor& bound_functor = std::get<expr::functor>(value->content);
            instantiate_.instantiate(right_var->index, bound_functor.id, bound_functor.args);
            continue;
        }

        const expr::functor& left_functor = std::get<expr::functor>(left.skeleton->content);
        const expr::functor& right_functor = std::get<expr::functor>(right.skeleton->content);
        if (left_functor.id != right_functor.id || left_functor.args.size() != right_functor.args.size())
            return false;

        // Pushed last to first so arguments are unified first to last, as a recursion would.
        for (size_t arg = left_functor.args.size(); arg-- > 0;)
            pending.emplace_back(
                framed_expr{left_functor.args[arg], left.frame_offset},
                framed_expr{right_functor.args[arg], right.frame_offset});
    }
    return true;
}

PUD_UNIFIER_TEMPLATE
bool PUD_UNIFIER::occurs(uint32_t var_index, const expr* normalized) {
    std::vector<const expr*> pending{normalized};
    while (!pending.empty()) {
        const expr* visited = pending.back();
        pending.pop_back();
        if (const expr::var* var = std::get_if<expr::var>(&visited->content)) {
            if (var->index == var_index)
                return true;
            continue;
        }
        for (const expr* arg : std::get<expr::functor>(visited->content).args)
            pending.push_back(arg);
    }
    return false;
}

#undef PUD_UNIFIER
#undef PUD_UNIFIER_TEMPLATE

#endif
