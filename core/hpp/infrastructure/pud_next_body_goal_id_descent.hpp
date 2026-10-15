#ifndef PUD_NEXT_BODY_GOAL_ID_DESCENT_HPP
#define PUD_NEXT_BODY_GOAL_ID_DESCENT_HPP

#include "value_objects/body_goal_id.hpp"
#include "value_objects/pud_node_id.hpp"

template<typename IGetNodeAddedBodyGoals>
struct pud_next_body_goal_id_descent {
    pud_next_body_goal_id_descent(IGetNodeAddedBodyGoals& get_node_added_body_goals, pud_node_id root);
    pud_next_body_goal_id_descent descend(pud_node_id child) const;
    body_goal_id next_body_goal_id() const;
private:
    IGetNodeAddedBodyGoals* get_node_added_body_goals_;
    body_goal_id next_body_goal_id_;
};

template<typename IGetNodeAddedBodyGoals>
pud_next_body_goal_id_descent<IGetNodeAddedBodyGoals>::pud_next_body_goal_id_descent(
    IGetNodeAddedBodyGoals& get_node_added_body_goals,
    pud_node_id root)
    : get_node_added_body_goals_(&get_node_added_body_goals)
    , next_body_goal_id_(get_node_added_body_goals.get(root).size()) {}

template<typename IGetNodeAddedBodyGoals>
pud_next_body_goal_id_descent<IGetNodeAddedBodyGoals>
pud_next_body_goal_id_descent<IGetNodeAddedBodyGoals>::descend(pud_node_id child) const {
    pud_next_body_goal_id_descent result = *this;
    result.next_body_goal_id_ += get_node_added_body_goals_->get(child).size();
    return result;
}

template<typename IGetNodeAddedBodyGoals>
body_goal_id pud_next_body_goal_id_descent<IGetNodeAddedBodyGoals>::next_body_goal_id() const {
    return next_body_goal_id_;
}

#endif
