#include "infrastructure/pud_node_added_body_goals.hpp"

pud_node_added_body_goals::pud_node_added_body_goals()
    : by_id_() {}

void pud_node_added_body_goals::store(
        const pud_rule_id* id,
        std::vector<const expr*> added_body_goals) {
    DEBUG_ASSERT(!by_id_.contains(id));
    by_id_.emplace(id, std::move(added_body_goals));
}

const std::vector<const expr*>&
pud_node_added_body_goals::get(const pud_rule_id* id) const {
    return by_id_.at(id);
}
