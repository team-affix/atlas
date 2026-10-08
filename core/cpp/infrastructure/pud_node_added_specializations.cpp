#include "infrastructure/pud_node_added_specializations.hpp"
#include "debug_assert.hpp"

const std::vector<pud_specialization>& pud_node_added_specializations::get(pud_node_id id) const {
    return specs_.at(id);
}

void pud_node_added_specializations::store(pud_node_id id, std::vector<pud_specialization> specs) {
    auto [_, inserted] = specs_.insert({id, std::move(specs)});
    DEBUG_ASSERT(inserted);
}
