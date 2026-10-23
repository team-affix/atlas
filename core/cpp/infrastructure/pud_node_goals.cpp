#include "infrastructure/pud_node_goals.hpp"
#include <utility>
#include "debug_assert.hpp"

void pud_node_goals::store(pud_global_node_id id, pud_goal_list goals) {
    goals_.store(id, std::move(goals));
}

const pud_goal_list& pud_node_goals::get(pud_global_node_id id) const {
    return goals_.get(id);
}

const expr* pud_node_goals::find(pud_global_node_id id, pud_body_goal_id goal) const {
    const expr* const* found = goals_.get(id).goals.find(goal);
    DEBUG_ASSERT(found != nullptr);
    return *found;
}
