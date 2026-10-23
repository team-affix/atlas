#include "infrastructure/pud_node_live_child_counts.hpp"
#include "debug_assert.hpp"

pud_node_live_child_counts::pud_node_live_child_counts()
    : counts_() {}

void pud_node_live_child_counts::store(pud_node_id id, size_t live_child_count) {
    if (counts_.size() <= id)
        counts_.resize(id + 1);
    DEBUG_ASSERT(!counts_.at(id).has_value());
    counts_.at(id) = live_child_count;
}

size_t pud_node_live_child_counts::get(pud_node_id id) const {
    return counts_.at(id).value();
}

size_t pud_node_live_child_counts::decrement(pud_node_id id) {
    size_t& count = counts_.at(id).value();
    DEBUG_ASSERT(count > 0);
    return --count;
}
