#include "infrastructure/pud_node_parent.hpp"

pud_node_parent::pud_node_parent()
    : by_id_() {}

void pud_node_parent::store(const pud_rule_id* child, const pud_rule_id* parent) {
    DEBUG_ASSERT(!by_id_.contains(child));
    by_id_.emplace(child, parent);
}

const pud_rule_id* pud_node_parent::get(const pud_rule_id* child) const {
    return by_id_.at(child);
}
