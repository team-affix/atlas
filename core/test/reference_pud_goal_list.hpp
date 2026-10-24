#ifndef REFERENCE_PUD_GOAL_LIST_HPP
#define REFERENCE_PUD_GOAL_LIST_HPP

#include "infrastructure/pud_module.hpp"
#include "value_objects/pud_goal_list.hpp"

// A node's open goals, rebuilt from its module alone: an axiom's body in order, or its parent's
// goals with the inference's call site closed and its body goals added.
inline pud_goal_list reference_pud_goal_list(const pud_module& module, pud_node_id node) {
    if (node < module.count_axioms()) {
        pud_goal_list goals{.goals = {}, .next_id = 0};
        for (const expr* goal : module.get_axiom(node).body)
            goals = goals.adding(goal);
        return goals;
    }
    const pud_inference& inference = module.get_inference(node);
    pud_goal_list goals = reference_pud_goal_list(module, inference.parent).closing(inference.call_site);
    for (const expr* goal : inference.body_goals)
        goals = goals.adding(goal);
    return goals;
}

#endif
