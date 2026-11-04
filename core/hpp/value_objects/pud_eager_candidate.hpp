#ifndef PUD_EAGER_CANDIDATE_HPP
#define PUD_EAGER_CANDIDATE_HPP

#include <compare>
#include <vector>
#include "value_objects/pud_global_node_id.hpp"

// A candidate found for a goal: the path from its axiom root down to the node it specialized to, and the leaves
// that witness it, the candidate node itself when it is a leaf, otherwise one leaf in each of two child subtrees.
struct pud_eager_candidate {
    std::vector<pud_global_node_id> path;
    std::vector<pud_global_node_id> witness_leaves;
    auto operator<=>(const pud_eager_candidate&) const = default;
};

#endif
