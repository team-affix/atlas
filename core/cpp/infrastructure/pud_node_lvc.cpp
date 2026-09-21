#include "infrastructure/pud_node_lvc.hpp"

pud_node_lvc::pud_node_lvc()
    : by_id_() {}

void pud_node_lvc::store(const pud_rule_id* id, uint32_t lvc) {
    DEBUG_ASSERT(!by_id_.contains(id));
    by_id_.emplace(id, lvc);
}

uint32_t pud_node_lvc::get(const pud_rule_id* id) const {
    return by_id_.at(id);
}
