#ifndef PUD_NEXT_BODY_GOAL_ID_DESCENDER_HPP
#define PUD_NEXT_BODY_GOAL_ID_DESCENDER_HPP

#include "value_objects/pud_next_body_goal_id_descent.hpp"
#include "value_objects/pud_node_id.hpp"

template<typename IGetNodeAddedBodyGoals>
struct pud_next_body_goal_id_descender {
    pud_next_body_goal_id_descender(IGetNodeAddedBodyGoals& get_node_added_body_goals);
    pud_next_body_goal_id_descent root_descent(pud_node_id root);
    pud_next_body_goal_id_descent descend(
        const pud_next_body_goal_id_descent& descent,
        pud_node_id parent,
        pud_node_id child);

private:
    IGetNodeAddedBodyGoals& get_node_added_body_goals_;
};

template<typename IGetNodeAddedBodyGoals>
pud_next_body_goal_id_descender<IGetNodeAddedBodyGoals>::pud_next_body_goal_id_descender(
    IGetNodeAddedBodyGoals& get_node_added_body_goals)
    : get_node_added_body_goals_(get_node_added_body_goals) {}

template<typename IGetNodeAddedBodyGoals>
pud_next_body_goal_id_descent pud_next_body_goal_id_descender<IGetNodeAddedBodyGoals>::root_descent(
    pud_node_id root) {
    const body_goal_id body_goal_count = get_node_added_body_goals_.get(root).size();
    return pud_next_body_goal_id_descent{.next_body_goal_id = body_goal_count};
}

template<typename IGetNodeAddedBodyGoals>
pud_next_body_goal_id_descent pud_next_body_goal_id_descender<IGetNodeAddedBodyGoals>::descend(
    const pud_next_body_goal_id_descent& descent,
    pud_node_id parent,
    pud_node_id child) {
    const body_goal_id body_goal_count = get_node_added_body_goals_.get(child).size();
    return pud_next_body_goal_id_descent{.next_body_goal_id = descent.next_body_goal_id + body_goal_count};
}

#endif
