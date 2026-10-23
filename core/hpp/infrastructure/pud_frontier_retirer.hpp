#ifndef PUD_FRONTIER_RETIRER_HPP
#define PUD_FRONTIER_RETIRER_HPP

#include <set>
#include <vector>
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_node_id.hpp"

// Takes leaves off the frontier, expanded or refuted, and follows every refutation that results: a
// query left without candidates refutes every leaf that holds it. Every leaf of a round is marked
// before any is invalidated, so no search reaches for a leaf that is gone but not yet reported.
template<
    typename IMarkLeafRefuted,
    typename IPropagateRefutation,
    typename IReleaseLeaf,
    typename IInvalidateQueryLeaves,
    typename IGetQueryHolders>
struct pud_frontier_retirer {
    pud_frontier_retirer(
        IMarkLeafRefuted& mark_leaf_refuted,
        IPropagateRefutation& propagate_refutation,
        IReleaseLeaf& release_leaf,
        IInvalidateQueryLeaves& invalidate_query_leaves,
        IGetQueryHolders& get_query_holders);
    std::vector<pud_node_id> retire(const std::vector<pud_node_id>& expanded, const std::vector<pud_node_id>& refuted);
private:
    IMarkLeafRefuted& mark_leaf_refuted_;
    IPropagateRefutation& propagate_refutation_;
    IReleaseLeaf& release_leaf_;
    IInvalidateQueryLeaves& invalidate_query_leaves_;
    IGetQueryHolders& get_query_holders_;
};

#define PUD_FRONTIER_RETIRER_TEMPLATE                                                      \
    template<typename IMarkLeafRefuted, typename IPropagateRefutation, typename IReleaseLeaf, \
             typename IInvalidateQueryLeaves, typename IGetQueryHolders>
#define PUD_FRONTIER_RETIRER                                                               \
    pud_frontier_retirer<IMarkLeafRefuted, IPropagateRefutation, IReleaseLeaf,               \
        IInvalidateQueryLeaves, IGetQueryHolders>

PUD_FRONTIER_RETIRER_TEMPLATE
PUD_FRONTIER_RETIRER::pud_frontier_retirer(
    IMarkLeafRefuted& mark_leaf_refuted,
    IPropagateRefutation& propagate_refutation,
    IReleaseLeaf& release_leaf,
    IInvalidateQueryLeaves& invalidate_query_leaves,
    IGetQueryHolders& get_query_holders)
    : mark_leaf_refuted_(mark_leaf_refuted)
    , propagate_refutation_(propagate_refutation)
    , release_leaf_(release_leaf)
    , invalidate_query_leaves_(invalidate_query_leaves)
    , get_query_holders_(get_query_holders) {}

// Returns every leaf refuted along the way. A leaf that holds a query is live, so the holders of a
// refuted query are never already refuted.
PUD_FRONTIER_RETIRER_TEMPLATE
std::vector<pud_node_id> PUD_FRONTIER_RETIRER::retire(
    const std::vector<pud_node_id>& expanded, const std::vector<pud_node_id>& refuted) {
    std::vector<pud_node_id> all_refuted;
    std::vector<pud_node_id> leaving = expanded;
    std::vector<pud_node_id> refuting = refuted;
    while (!leaving.empty() || !refuting.empty()) {
        for (pud_node_id leaf : refuting) {
            mark_leaf_refuted_.mark_refuted(leaf);
            propagate_refutation_.refute(leaf);
            all_refuted.push_back(leaf);
            leaving.push_back(leaf);
        }
        for (pud_node_id leaf : leaving)
            release_leaf_.release(leaf);
        std::set<pud_node_id> holders;
        for (pud_mhq_head_id head_id : invalidate_query_leaves_.invalidate_leaves(leaving)) {
            const std::vector<pud_node_id> held_by = get_query_holders_.get_holders(head_id);
            holders.insert(held_by.begin(), held_by.end());
        }
        leaving.clear();
        refuting.assign(holders.begin(), holders.end());
    }
    return all_refuted;
}

#undef PUD_FRONTIER_RETIRER
#undef PUD_FRONTIER_RETIRER_TEMPLATE

#endif
