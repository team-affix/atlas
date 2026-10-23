#include "infrastructure/pud_node_live_child_counts.hpp"
#include "debug_assert.hpp"

pud_node_live_child_counts::pud_node_live_child_counts()
    : counts_() {}

void pud_node_live_child_counts::increment(pud_global_node_id id) {
    ++counts_[id];
}

size_t pud_node_live_child_counts::decrement(pud_global_node_id id) {
    size_t& count = counts_.at(id);
    DEBUG_ASSERT(count > 0);
    return --count;
}
