#ifndef PUD_FRONTIER_REFUTER_HPP
#define PUD_FRONTIER_REFUTER_HPP

#include <set>
#include <vector>
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_node_id.hpp"

// Marks leaves refuted, releases their queries, and follows query invalidation until every holder of
// a head with no candidates left is refuted. Returns every leaf refuted in the cascade.
template<
    typename IMarkLeafRefuted,
    typename IPropagateRefutation,
    typename IReleaseLeaf,
    typename IInvalidateQueryLeaves,
    typename IGetQueryHolders>
struct pud_frontier_refuter {
    pud_frontier_refuter(
        IMarkLeafRefuted& mark_leaf_refuted,
        IPropagateRefutation& propagate_refutation,
        IReleaseLeaf& release_leaf,
        IInvalidateQueryLeaves& invalidate_query_leaves,
        IGetQueryHolders& get_query_holders);
    std::vector<pud_node_id> refute(const std::vector<pud_node_id>& leaving, const std::vector<pud_node_id>& refuted);
private:
    IMarkLeafRefuted& mark_leaf_refuted_;
    IPropagateRefutation& propagate_refutation_;
    IReleaseLeaf& release_leaf_;
    IInvalidateQueryLeaves& invalidate_query_leaves_;
    IGetQueryHolders& get_query_holders_;
};

#define PUD_FRONTIER_REFUTER_TEMPLATE                                                      \
    template<typename IMarkLeafRefuted, typename IPropagateRefutation, typename IReleaseLeaf, \
             typename IInvalidateQueryLeaves, typename IGetQueryHolders>
#define PUD_FRONTIER_REFUTER                                                               \
    pud_frontier_refuter<IMarkLeafRefuted, IPropagateRefutation, IReleaseLeaf,               \
        IInvalidateQueryLeaves, IGetQueryHolders>

PUD_FRONTIER_REFUTER_TEMPLATE
PUD_FRONTIER_REFUTER::pud_frontier_refuter(
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

PUD_FRONTIER_REFUTER_TEMPLATE
std::vector<pud_node_id> PUD_FRONTIER_REFUTER::refute(
    const std::vector<pud_node_id>& leaving, const std::vector<pud_node_id>& refuted) {
    std::vector<pud_node_id> all_refuted;
    std::vector<pud_node_id> releasing = leaving;
    std::vector<pud_node_id> refuting = refuted;
    while (!releasing.empty() || !refuting.empty()) {
        for (pud_node_id leaf : refuting) {
            mark_leaf_refuted_.mark_refuted(leaf);
            propagate_refutation_.refute(leaf);
            all_refuted.push_back(leaf);
            releasing.push_back(leaf);
        }
        for (pud_node_id leaf : releasing)
            release_leaf_.release(leaf);
        std::set<pud_node_id> holders;
        for (pud_mhq_head_id head_id : invalidate_query_leaves_.invalidate_leaves(releasing)) {
            const std::vector<pud_node_id> held_by = get_query_holders_.get_holders(head_id);
            holders.insert(held_by.begin(), held_by.end());
        }
        releasing.clear();
        refuting.assign(holders.begin(), holders.end());
    }
    return all_refuted;
}

#undef PUD_FRONTIER_REFUTER
#undef PUD_FRONTIER_REFUTER_TEMPLATE

#endif
