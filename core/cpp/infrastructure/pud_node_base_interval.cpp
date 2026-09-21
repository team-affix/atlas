#include "infrastructure/pud_node_base_interval.hpp"

pud_node_base_interval::pud_node_base_interval()
    : by_id_() {}

void pud_node_base_interval::store(const pud_rule_id* id, om_interval interval) {
    DEBUG_ASSERT(!by_id_.contains(id));
    by_id_.emplace(id, interval);
}

const om_interval& pud_node_base_interval::get(const pud_rule_id* id) const {
    return by_id_.at(id);
}
