#include "infrastructure/pud_node_live_child_counts.hpp"
#include "debug_assert.hpp"

pud_node_live_child_counts::pud_node_live_child_counts()
    : counts_() {}

void pud_node_live_child_counts::store(pud_global_node_id id, size_t live_child_count) {
    [[maybe_unused]] const bool stored = counts_.emplace(id, live_child_count).second;
    DEBUG_ASSERT(stored);
}

size_t pud_node_live_child_counts::get(pud_global_node_id id) const {
    return counts_.at(id);
}

size_t pud_node_live_child_counts::decrement(pud_global_node_id id) {
    size_t& count = counts_.at(id);
    DEBUG_ASSERT(count > 0);
    return --count;
}
