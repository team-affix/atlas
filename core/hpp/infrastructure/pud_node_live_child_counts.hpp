#ifndef PUD_NODE_LIVE_CHILD_COUNTS_HPP
#define PUD_NODE_LIVE_CHILD_COUNTS_HPP

#include <cstddef>
#include <unordered_map>
#include "value_objects/pud_node_id.hpp"

// Only interior nodes have an entry; it is stored once, when the node gets its children.
struct pud_node_live_child_counts {
    size_t get(pud_node_id id) const;
    void store(pud_node_id id, size_t live_child_count);
    size_t decrement(pud_node_id id);
private:
    std::unordered_map<pud_node_id, size_t> live_child_counts_;
};

#endif
