#ifndef PUD_NODE_DEACTIVATOR_HPP
#define PUD_NODE_DEACTIVATOR_HPP

#include <vector>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_global_node_id.hpp"

// Drops a node's active state and query holds, removes orphan queries, and stops the node witnessing
// any candidate. Returns the nodes still holding a query that was left without a candidate.
template<
    typename IGetActiveState,
    typename IEraseActiveState,
    typename IReleaseQuery,
    typename IRemoveQuery,
    typename IInvalidateQueryLeaf,
    typename IGetQueryHolders>
struct pud_node_deactivator {
    pud_node_deactivator(
        IGetActiveState& get_active_state,
        IEraseActiveState& erase_active_state,
        IReleaseQuery& release_query,
        IRemoveQuery& remove_query,
        IInvalidateQueryLeaf& invalidate_query_leaf,
        IGetQueryHolders& get_query_holders);
    std::vector<pud_global_node_id> deactivate(pud_global_node_id node);
private:
    IGetActiveState& get_active_state_;
    IEraseActiveState& erase_active_state_;
    IReleaseQuery& release_query_;
    IRemoveQuery& remove_query_;
    IInvalidateQueryLeaf& invalidate_query_leaf_;
    IGetQueryHolders& get_query_holders_;
};

#define PUD_NODE_DEACTIVATOR_TEMPLATE                                                      \
    template<typename IGetActiveState, typename IEraseActiveState, typename IReleaseQuery, \
             typename IRemoveQuery, typename IInvalidateQueryLeaf, typename IGetQueryHolders>
#define PUD_NODE_DEACTIVATOR                                                               \
    pud_node_deactivator<IGetActiveState, IEraseActiveState, IReleaseQuery, IRemoveQuery,  \
        IInvalidateQueryLeaf, IGetQueryHolders>

PUD_NODE_DEACTIVATOR_TEMPLATE
PUD_NODE_DEACTIVATOR::pud_node_deactivator(
    IGetActiveState& get_active_state,
    IEraseActiveState& erase_active_state,
    IReleaseQuery& release_query,
    IRemoveQuery& remove_query,
    IInvalidateQueryLeaf& invalidate_query_leaf,
    IGetQueryHolders& get_query_holders)
    : get_active_state_(get_active_state)
    , erase_active_state_(erase_active_state)
    , release_query_(release_query)
    , remove_query_(remove_query)
    , invalidate_query_leaf_(invalidate_query_leaf)
    , get_query_holders_(get_query_holders) {}

PUD_NODE_DEACTIVATOR_TEMPLATE
std::vector<pud_global_node_id> PUD_NODE_DEACTIVATOR::deactivate(pud_global_node_id node) {
    for (const auto& [goal, head_id] : get_active_state_.get(node).queries) {
        if (!release_query_.release(head_id, node))
            continue;
        remove_query_.remove(head_id);
    }
    erase_active_state_.erase(node);
    std::vector<pud_global_node_id> stranded;
    for (pud_mhq_head_id exhausted : invalidate_query_leaf_.invalidate_leaf(node)) {
        const std::vector<pud_global_node_id> holders = get_query_holders_.get_holders(exhausted);
        stranded.insert(stranded.end(), holders.begin(), holders.end());
    }
    return stranded;
}

#undef PUD_NODE_DEACTIVATOR
#undef PUD_NODE_DEACTIVATOR_TEMPLATE

#endif
