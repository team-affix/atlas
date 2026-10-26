#ifndef REFERENCE_PUD_ANSWER_HPP
#define REFERENCE_PUD_ANSWER_HPP

#include <algorithm>
#include <vector>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/ubs_bind_map.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/pud_ubs_state.hpp"

inline const expr* reference_pud_answer_normalize(const ubs_bind_map& bind_map, expr_pool& pool, const expr* skeleton) {
    if (const expr::var* variable = std::get_if<expr::var>(&skeleton->content)) {
        const ubs_class variable_class = bind_map.find(variable->index);
        if (!variable_class.value.has_value())
            return pool.make_var(variable_class.rep);
        return reference_pud_answer_normalize(bind_map, pool, variable_class.value->parent);
    }
    const expr::functor& term = std::get<expr::functor>(skeleton->content);
    std::vector<const expr*> args;
    for (const expr* arg : term.args)
        args.push_back(reference_pud_answer_normalize(bind_map, pool, arg));
    return pool.make_functor(term.id, args);
}

inline const expr* reference_pud_answer(const pud_module& module, pud_node_id node, expr_pool& pool) {
    pud_node_id axiom = node;
    while (axiom >= module.count_axioms())
        axiom = module.get_inference(axiom).parent;

    std::vector<pud_node_id> path;
    for (pud_node_id current = node; current >= module.count_axioms();) {
        path.push_back(current);
        current = module.get_inference(current).parent;
    }
    std::reverse(path.begin(), path.end());

    ubs_bind_map bind_map{pud_ubs_state{}};
    for (pud_node_id inference_node : path) {
        for (const pud_substitution& substitution : module.get_inference(inference_node).substitutions) {
            const ubs_class target = bind_map.find(substitution.var_idx);
            if (const expr::var* value_var = std::get_if<expr::var>(&substitution.value->content)) {
                bind_map.equate(target, bind_map.find(value_var->index));
                continue;
            }
            bind_map.instantiate(target, framed_functor{std::get_if<expr::functor>(&(substitution.value)->content), 0});
        }
    }
    return reference_pud_answer_normalize(bind_map, pool, module.get_axiom(axiom).head);
}

#endif
