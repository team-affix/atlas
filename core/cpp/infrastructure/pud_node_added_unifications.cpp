#include "infrastructure/pud_node_added_unifications.hpp"

pud_node_added_unifications::pud_node_added_unifications()
    : by_id_() {}

void pud_node_added_unifications::store(
        const pud_rule_id* id,
        std::vector<pud_added_unification> added_unifications) {
    DEBUG_ASSERT(!by_id_.contains(id));
    by_id_.emplace(id, std::move(added_unifications));
}

const std::vector<pud_added_unification>&
pud_node_added_unifications::get(const pud_rule_id* id) const {
    return by_id_.at(id);
}
