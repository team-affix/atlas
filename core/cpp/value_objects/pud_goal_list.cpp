#include "value_objects/pud_goal_list.hpp"
#include <algorithm>
#include <cstddef>

pud_goal_list pud_goal_list::closing(pud_body_goal_id goal) const {
    const auto closed = std::lower_bound(goals.begin(), goals.end(), pud_body_goal{.id = goal, .goal = nullptr});
    return pud_goal_list{.goals = goals.erase(static_cast<size_t>(closed - goals.begin())), .next_id = next_id};
}

pud_goal_list pud_goal_list::adding(const expr* goal) const {
    return pud_goal_list{
        .goals = goals.push_back(pud_body_goal{.id = next_id, .goal = goal}),
        .next_id = next_id + 1,
    };
}
