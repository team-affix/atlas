#include "infrastructure/pud_node_added_body_goals.hpp"
#include "debug_assert.hpp"

const std::vector<const expr*>& pud_node_added_body_goals::get(pud_node_id id) const {
    return goals_.at(id);
}

void pud_node_added_body_goals::store(pud_node_id id, std::vector<const expr*> goals) {
    auto [_, inserted] = goals_.insert({id, std::move(goals)});
    DEBUG_ASSERT(inserted);
}
