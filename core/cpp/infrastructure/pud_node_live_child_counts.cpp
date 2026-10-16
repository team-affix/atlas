#include "infrastructure/pud_node_live_child_counts.hpp"
#include "debug_assert.hpp"

size_t pud_node_live_child_counts::get(pud_node_id id) const {
    return live_child_counts_.at(id);
}

void pud_node_live_child_counts::store(pud_node_id id, size_t live_child_count) {
    live_child_counts_.insert_or_assign(id, live_child_count);
}

size_t pud_node_live_child_counts::decrement(pud_node_id id) {
    size_t& live_child_count = live_child_counts_.at(id);
    DEBUG_ASSERT(live_child_count > 0);
    return --live_child_count;
}
