#ifndef PUD_CANDIDATE_TRAIL_HPP
#define PUD_CANDIDATE_TRAIL_HPP

#include <compare>
#include <vector>
#include "value_objects/pud_global_node_id.hpp"

// What a query found for one candidate, as ids only: the node it specialized to and the leaves that witness it, the
// candidate node itself when it is a leaf, otherwise one leaf in each of two child subtrees. Its path is the
// candidate node's ancestors.
struct pud_candidate_trail {
    pud_global_node_id candidate;
    std::vector<pud_global_node_id> witness_leaves;
    auto operator<=>(const pud_candidate_trail&) const = default;
};

#endif
