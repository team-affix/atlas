#ifndef PUD_LEAF_DEACTIVATOR_HPP
#define PUD_LEAF_DEACTIVATOR_HPP

#include <vector>
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_node_id.hpp"

// Takes one leaf off the frontier without marking it refuted: releases its state and queries, and
// reports every query head that lost its last holder.
template<typename IReleaseLeaf, typename IInvalidateQueryLeaf>
struct pud_leaf_deactivator {
    pud_leaf_deactivator(IReleaseLeaf& release_leaf, IInvalidateQueryLeaf& invalidate_query_leaf);
    std::vector<pud_mhq_head_id> deactivate(pud_node_id leaf);
private:
    IReleaseLeaf& release_leaf_;
    IInvalidateQueryLeaf& invalidate_query_leaf_;
};

template<typename IRL, typename IIQL>
pud_leaf_deactivator<IRL, IIQL>::pud_leaf_deactivator(IRL& release_leaf, IIQL& invalidate_query_leaf)
    : release_leaf_(release_leaf)
    , invalidate_query_leaf_(invalidate_query_leaf) {}

template<typename IRL, typename IIQL>
std::vector<pud_mhq_head_id> pud_leaf_deactivator<IRL, IIQL>::deactivate(pud_node_id leaf) {
    release_leaf_.release(leaf);
    return invalidate_query_leaf_.invalidate_leaf(leaf);
}

#endif
