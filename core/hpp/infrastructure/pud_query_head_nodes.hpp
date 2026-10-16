#ifndef PUD_QUERY_HEAD_NODES_HPP
#define PUD_QUERY_HEAD_NODES_HPP

#include <unordered_map>
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_node_id.hpp"

// The node at which each MHQ head was opened or forked.
struct pud_query_head_nodes {
    pud_node_id get(pud_mhq_head_id head_id) const;
    void store(pud_mhq_head_id head_id, pud_node_id node);
private:
    std::unordered_map<pud_mhq_head_id, pud_node_id> nodes_;
};

#endif
