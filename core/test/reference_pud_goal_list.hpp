#ifndef REFERENCE_PUD_GOAL_LIST_HPP
#define REFERENCE_PUD_GOAL_LIST_HPP

#include "infrastructure/pud_module.hpp"
#include "value_objects/pud_goal_list.hpp"

// A node's open goals, rebuilt from its module alone: an axiom's body in order, or its parent's
// goals with the inference's call site closed and its body goals added.
inline pud_goal_list reference_pud_goal_list(const pud_module& module, pud_node_id node) {
    if (node < module.count_axioms()) {
        immer::map<pud_body_goal_id, const expr*> goals;
        pud_body_goal_id next_id = 0;
        for (const expr* goal : module.get_axiom(node).body) {
            goals = goals.set(next_id, goal);
            next_id = next_id + 1;
        }
        return pud_goal_list{.goals = goals, .next_id = next_id};
    }
    const pud_inference& inference = module.get_inference(node);
    const pud_goal_list parent_goals = reference_pud_goal_list(module, inference.parent);
    immer::map<pud_body_goal_id, const expr*> goals = parent_goals.goals.erase(inference.call_site);
    pud_body_goal_id next_id = parent_goals.next_id;
    for (const expr* goal : inference.body_goals) {
        goals = goals.set(next_id, goal);
        next_id = next_id + 1;
    }
    return pud_goal_list{.goals = goals, .next_id = next_id};
}

#endif
