#include "infrastructure/pud_node_goals.hpp"
#include <algorithm>
#include <utility>
#include "value_objects/pud_body_goal.hpp"
#include "debug_assert.hpp"

void pud_node_goals::store(pud_node_id id, pud_goal_list goals) {
    goals_.store(id, std::move(goals));
}

const pud_goal_list& pud_node_goals::get(pud_node_id id) const {
    return goals_.get(id);
}

const expr* pud_node_goals::find(pud_node_id id, pud_body_goal_id goal) const {
    const immer::flex_vector<pud_body_goal>& goals = goals_.get(id).goals;
    const auto found = std::lower_bound(goals.begin(), goals.end(), pud_body_goal{.id = goal, .goal = nullptr});
    DEBUG_ASSERT(found != goals.end() && found->id == goal);
    return found->goal;
}
