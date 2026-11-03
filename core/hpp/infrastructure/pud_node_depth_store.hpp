#ifndef PUD_NODE_DEPTH_STORE_HPP
#define PUD_NODE_DEPTH_STORE_HPP

#include <cstddef>
#include <unordered_map>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_global_node_id_hash.hpp"

// Live-plane depth of loaded nodes: the number of inference edges up to the root axiom. Never
// persisted; the depth seeder records every stored node on load and the depth extender adds the live
// children of each unfold.
struct pud_node_depth_store {
    pud_node_depth_store();
    void insert(pud_global_node_id node, size_t depth);
    size_t get_depth(pud_global_node_id node) const;
private:
    std::unordered_map<pud_global_node_id, size_t, pud_global_node_id_hash> depths_;
};

#endif
