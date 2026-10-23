#include "value_objects/pud_goal_list.hpp"

pud_goal_list pud_goal_list::closing(pud_body_goal_id goal) const {
    return pud_goal_list{.goals = goals.erase(goal), .next_id = next_id};
}

pud_goal_list pud_goal_list::adding(const expr* goal) const {
    return pud_goal_list{.goals = goals.set(next_id, goal), .next_id = next_id + 1};
}
