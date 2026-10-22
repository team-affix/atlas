#include "infrastructure/pud_node_added_var_count.hpp"
#include "debug_assert.hpp"

uint32_t pud_node_added_var_count::get(pud_node_id id) const {
    return var_counts_.at(id);
}

void pud_node_added_var_count::store(pud_node_id id, uint32_t var_count) {
    auto [_, inserted] = var_counts_.insert({id, var_count});
    DEBUG_ASSERT(inserted);
}
