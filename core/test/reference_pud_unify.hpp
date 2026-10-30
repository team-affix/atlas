#ifndef REFERENCE_PUD_UNIFY_HPP
#define REFERENCE_PUD_UNIFY_HPP

#include <cstdint>
#include <optional>
#include "pud_term_stack.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/moduled_functor.hpp"

inline bool reference_pud_occurs(uint32_t var_index, const expr* term) {
    if (const expr::var* var = std::get_if<expr::var>(&term->content))
        return var->index == var_index;
    const expr::functor& functor = std::get<expr::functor>(term->content);
    for (const expr* arg : functor.args) {
        if (reference_pud_occurs(var_index, arg))
            return true;
    }
    return false;
}

inline framed_expr reference_pud_deref(pud_term_stack& stack, uint32_t global) {
    if (const std::optional<moduled_functor> value = stack.bind_map.find(global).value) {
        const expr::functor& f = *value->functor;
        const expr* reconstructed = stack.pool.make_functor(f.id, f.args);
        return framed_expr{reconstructed, value->frame_offset};
    }
    return framed_expr{stack.pool.make_var(stack.bind_map.find(global).rep), 0u};
}

inline bool reference_pud_unify_terms(
    pud_term_stack& stack,
    framed_expr lhs,
    framed_expr rhs) {
    if (const expr::var* var = std::get_if<expr::var>(&lhs.skeleton->content))
        lhs = reference_pud_deref(stack, stack.globalize.globalize(lhs.frame_offset, var->index));
    if (const expr::var* var = std::get_if<expr::var>(&rhs.skeleton->content))
        rhs = reference_pud_deref(stack, stack.globalize.globalize(rhs.frame_offset, var->index));

    const expr::var* lhs_var = std::get_if<expr::var>(&lhs.skeleton->content);
    const expr::var* rhs_var = std::get_if<expr::var>(&rhs.skeleton->content);

    const uint32_t lhs_global = lhs_var
        ? stack.globalize.globalize(lhs.frame_offset, lhs_var->index) : 0;
    const uint32_t rhs_global = rhs_var
        ? stack.globalize.globalize(rhs.frame_offset, rhs_var->index) : 0;

    if (lhs_var && rhs_var && lhs_global == rhs_global)
        return true;

    if (lhs_var && rhs_var) {
        stack.bind_map.equate(stack.bind_map.find(lhs_var->index), stack.bind_map.find(rhs_var->index));
        return true;
    }
    if (lhs_var) {
        const expr* value = stack.normalizer.normalize(rhs);
        if (reference_pud_occurs(lhs_var->index, value))
            return false;
        stack.bind_map.instantiate(stack.bind_map.find(lhs_var->index), moduled_functor{std::get_if<expr::functor>(&(value)->content), 0u, 0});
        return true;
    }
    if (rhs_var) {
        const expr* value = stack.normalizer.normalize(lhs);
        if (reference_pud_occurs(rhs_var->index, value))
            return false;
        stack.bind_map.instantiate(stack.bind_map.find(rhs_var->index), moduled_functor{std::get_if<expr::functor>(&(value)->content), 0u, 0});
        return true;
    }

    const expr::functor& lf = std::get<expr::functor>(lhs.skeleton->content);
    const expr::functor& rf = std::get<expr::functor>(rhs.skeleton->content);
    if (lf.id != rf.id || lf.args.size() != rf.args.size())
        return false;
    for (size_t arg_index = 0; arg_index < lf.args.size(); ++arg_index) {
        if (!reference_pud_unify_terms(
                stack,
                framed_expr{lf.args[arg_index], lhs.frame_offset},
                framed_expr{rf.args[arg_index], rhs.frame_offset}))
            return false;
    }
    return true;
}

#endif
