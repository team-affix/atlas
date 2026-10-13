#ifndef PUD_UNIFIER_HPP
#define PUD_UNIFIER_HPP

#include <cstddef>
#include <cstdint>
#include <variant>
#include "value_objects/framed_expr.hpp"
#include "value_objects/expr.hpp"

// Unifies framed terms. Before binding a variable's class to a functor, the
// functor is normalized and checked for the variable (occurs check).
template<
    typename IGlobalize,
    typename INormalize,
    typename IResolve,
    typename IEquate,
    typename IInstantiate>
struct unifier {
    unifier(
        IGlobalize& globalize,
        INormalize& normalize,
        IResolve& resolve,
        IEquate& equate,
        IInstantiate& instantiate);
    bool unify(framed_expr lhs, framed_expr rhs);

private:
    // Whether a variable with this global index occurs in the normalized term.
    bool occurs(uint32_t var_index, const expr* term);

    IGlobalize& globalize_;
    INormalize& normalize_;
    IResolve& resolve_;
    IEquate& equate_;
    IInstantiate& instantiate_;
};

#define UNIFIER_TEMPLATE                                                         \
    template<typename IGlobalize, typename INormalize, typename IResolve,           \
             typename IEquate, typename IInstantiate>
#define UNIFIER unifier<IGlobalize, INormalize, IResolve, IEquate, IInstantiate>

UNIFIER_TEMPLATE
UNIFIER::unifier(
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

UNIFIER_TEMPLATE
bool UNIFIER::unify(framed_expr lhs, framed_expr rhs) {
    // Replace each variable with its class's value: a functor, or the rep var.
    // Values are in global indices, so they carry offset 0.
    if (const expr::var* var = std::get_if<expr::var>(&lhs.skeleton->content))
        lhs = {resolve_.resolve(globalize_.globalize(lhs.frame_offset, var->index)), 0u};
    if (const expr::var* var = std::get_if<expr::var>(&rhs.skeleton->content))
        rhs = {resolve_.resolve(globalize_.globalize(rhs.frame_offset, var->index)), 0u};

    // Same hash-consed skeleton in the same frame: same term.
    if (lhs.skeleton == rhs.skeleton && lhs.frame_offset == rhs.frame_offset)
        return true;

    const expr::var* lhs_var = std::get_if<expr::var>(&lhs.skeleton->content);
    const expr::var* rhs_var = std::get_if<expr::var>(&rhs.skeleton->content);

    if (lhs_var && rhs_var) {
        equate_.equate(lhs_var->index, rhs_var->index);
        return true;
    }
    if (lhs_var) {
        const expr* value = normalize_.normalize(rhs);
        if (occurs(lhs_var->index, value))
            return false;
        instantiate_.instantiate(lhs_var->index, value);
        return true;
    }
    if (rhs_var) {
        const expr* value = normalize_.normalize(lhs);
        if (occurs(rhs_var->index, value))
            return false;
        instantiate_.instantiate(rhs_var->index, value);
        return true;
    }

    const expr::functor& lhs_functor = std::get<expr::functor>(lhs.skeleton->content);
    const expr::functor& rhs_functor = std::get<expr::functor>(rhs.skeleton->content);
    if (lhs_functor.id != rhs_functor.id || lhs_functor.args.size() != rhs_functor.args.size())
        return false;

    for (size_t i = 0; i < lhs_functor.args.size(); ++i)
        if (!unify({lhs_functor.args[i], lhs.frame_offset},
                   {rhs_functor.args[i], rhs.frame_offset}))
            return false;
    return true;
}

UNIFIER_TEMPLATE
bool UNIFIER::occurs(uint32_t var_index, const expr* term) {
    if (const expr::var* var = std::get_if<expr::var>(&term->content)) {
        return var->index == var_index;
    }
    const expr::functor& functor = std::get<expr::functor>(term->content);
    for (const expr* arg : functor.args)
        if (occurs(var_index, arg))
            return true;
    return false;
}

#undef UNIFIER
#undef UNIFIER_TEMPLATE

#endif