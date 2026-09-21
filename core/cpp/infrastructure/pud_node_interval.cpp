#include "infrastructure/pud_node_interval.hpp"

pud_node_interval::pud_node_interval()
    : by_id_() {}

void pud_node_interval::store(const pud_rule_id* id, om_interval interval) {
    by_id_.insert_or_assign(id, interval);
}

const om_interval& pud_node_interval::get(const pud_rule_id* id) const {
    return by_id_.at(id);
}

bool pud_node_interval::contains(const pud_rule_id* id) const {
    return by_id_.contains(id);
}
