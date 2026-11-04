#ifndef PUD_LODESTAR_SMALLEST_GOAL_CHOOSER_HPP
#define PUD_LODESTAR_SMALLEST_GOAL_CHOOSER_HPP

#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "debug_assert.hpp"

// Lodestar's heuristic for which goal of a leaf to unfold: its smallest open goal id. Every goal of a live,
// unrefuted leaf has candidates, since an exhausted query refutes the leaves holding it.
template<typename IGetNodeGoals>
struct pud_lodestar_smallest_goal_chooser {
    pud_lodestar_smallest_goal_chooser(IGetNodeGoals& get_node_goals);
    pud_body_goal_id choose(pud_global_node_id leaf) const;
private:
    IGetNodeGoals& get_node_goals_;
};

#define PUD_LODESTAR_SMALLEST_GOAL_CHOOSER_TEMPLATE template<typename IGetNodeGoals>
#define PUD_LODESTAR_SMALLEST_GOAL_CHOOSER          pud_lodestar_smallest_goal_chooser<IGetNodeGoals>

PUD_LODESTAR_SMALLEST_GOAL_CHOOSER_TEMPLATE
PUD_LODESTAR_SMALLEST_GOAL_CHOOSER::pud_lodestar_smallest_goal_chooser(IGetNodeGoals& get_node_goals)
    : get_node_goals_(get_node_goals) {}

PUD_LODESTAR_SMALLEST_GOAL_CHOOSER_TEMPLATE
pud_body_goal_id PUD_LODESTAR_SMALLEST_GOAL_CHOOSER::choose(pud_global_node_id leaf) const {
    const pud_goal_list& goals = get_node_goals_.get(leaf);
    DEBUG_ASSERT(!goals.goals.empty());
    pud_body_goal_id smallest = goals.next_id;
    for (const auto& [goal, goal_expr] : goals.goals) {
        if (goal >= smallest)
            continue;
        smallest = goal;
    }
    return smallest;
}

#undef PUD_LODESTAR_SMALLEST_GOAL_CHOOSER
#undef PUD_LODESTAR_SMALLEST_GOAL_CHOOSER_TEMPLATE

#endif
