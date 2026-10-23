#ifndef PUD_NODE_LIVE_CHILD_COUNTS_HPP
#define PUD_NODE_LIVE_CHILD_COUNTS_HPP

#include <cstddef>
#include <unordered_map>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_global_node_id_hash.hpp"

// How many children of each interior node are still alive; stored when the node gets its children.
struct pud_node_live_child_counts {
    pud_node_live_child_counts();
    void store(pud_global_node_id id, size_t live_child_count);
    size_t get(pud_global_node_id id) const;
    size_t decrement(pud_global_node_id id);
private:
    std::unordered_map<pud_global_node_id, size_t, pud_global_node_id_hash> counts_;
};

#endif
