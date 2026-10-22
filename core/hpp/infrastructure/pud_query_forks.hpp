#ifndef PUD_QUERY_FORKS_HPP
#define PUD_QUERY_FORKS_HPP

#include <unordered_map>
#include <unordered_set>
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_node_id.hpp"

// For each node, the query heads that were forked there. A query forked at a node is held by none of
// that node's descendants, only by its forks.
struct pud_query_forks {
    pud_query_forks();
    bool check_query_forked(pud_node_id node, pud_mhq_head_id head_id) const;
    void mark_query_forked(pud_node_id node, pud_mhq_head_id head_id);
private:
    std::unordered_map<pud_node_id, std::unordered_set<pud_mhq_head_id>> forked_heads_;
};

#endif
