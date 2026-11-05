#ifndef PUD_SMALLEST_GOAL_CHOOSER_HPP
#define PUD_SMALLEST_GOAL_CHOOSER_HPP

#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "debug_assert.hpp"

// Chooses a leaf's smallest open goal id. The leaf has an open goal: only unsolved leaves are unfolded.
template<typename IGetNodeGoals>
struct pud_smallest_goal_chooser {
    pud_smallest_goal_chooser(IGetNodeGoals& get_node_goals);
    pud_body_goal_id choose(pud_global_node_id leaf) const;
private:
    IGetNodeGoals& get_node_goals_;
};

template<typename IGetNodeGoals>
pud_smallest_goal_chooser<IGetNodeGoals>::pud_smallest_goal_chooser(IGetNodeGoals& get_node_goals)
    : get_node_goals_(get_node_goals) {}

template<typename IGetNodeGoals>
pud_body_goal_id pud_smallest_goal_chooser<IGetNodeGoals>::choose(pud_global_node_id leaf) const {
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

#endif
