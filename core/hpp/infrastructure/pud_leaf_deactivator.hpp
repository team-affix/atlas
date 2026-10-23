#ifndef PUD_LEAF_DEACTIVATOR_HPP
#define PUD_LEAF_DEACTIVATOR_HPP

#include <vector>
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_node_id.hpp"

// Takes one leaf off the frontier without marking it refuted: releases its state and queries, and
// reports every query head that lost its last holder.
template<typename IReleaseLeaf, typename IInvalidateQueryLeaves>
struct pud_leaf_deactivator {
    pud_leaf_deactivator(IReleaseLeaf& release_leaf, IInvalidateQueryLeaves& invalidate_query_leaves);
    std::vector<pud_mhq_head_id> deactivate(pud_node_id leaf);
private:
    IReleaseLeaf& release_leaf_;
    IInvalidateQueryLeaves& invalidate_query_leaves_;
};

template<typename IRL, typename IIQL>
pud_leaf_deactivator<IRL, IIQL>::pud_leaf_deactivator(IRL& release_leaf, IIQL& invalidate_query_leaves)
    : release_leaf_(release_leaf)
    , invalidate_query_leaves_(invalidate_query_leaves) {}

template<typename IRL, typename IIQL>
std::vector<pud_mhq_head_id> pud_leaf_deactivator<IRL, IIQL>::deactivate(pud_node_id leaf) {
    release_leaf_.release(leaf);
    return invalidate_query_leaves_.invalidate_leaves({leaf});
}

#endif
