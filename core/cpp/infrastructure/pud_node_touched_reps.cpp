#include "infrastructure/pud_node_touched_reps.hpp"

pud_node_touched_reps::pud_node_touched_reps()
    : by_id_() {}

void pud_node_touched_reps::store(const pud_rule_id* id, std::vector<uint32_t> reps) {
    by_id_.insert_or_assign(id, std::move(reps));
}

const std::vector<uint32_t>& pud_node_touched_reps::get(const pud_rule_id* id) const {
    return by_id_.at(id);
}
