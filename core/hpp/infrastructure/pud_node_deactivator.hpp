#ifndef PUD_NODE_DEACTIVATOR_HPP
#define PUD_NODE_DEACTIVATOR_HPP

#include <vector>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_node_id.hpp"

// Drops a node's active state and query holds, removes orphan queries, and reports query heads that
// lost their last candidate.
template<
    typename IGetActiveState,
    typename IEraseActiveState,
    typename IReleaseQuery,
    typename IRemoveQuery,
    typename IInvalidateQueryLeaf>
struct pud_node_deactivator {
    pud_node_deactivator(
        IGetActiveState& get_active_state,
        IEraseActiveState& erase_active_state,
        IReleaseQuery& release_query,
        IRemoveQuery& remove_query,
        IInvalidateQueryLeaf& invalidate_query_leaf);
    std::vector<pud_mhq_head_id> deactivate(pud_node_id node);
private:
    IGetActiveState& get_active_state_;
    IEraseActiveState& erase_active_state_;
    IReleaseQuery& release_query_;
    IRemoveQuery& remove_query_;
    IInvalidateQueryLeaf& invalidate_query_leaf_;
};

template<typename IGAS, typename IEAS, typename IRelQ, typename IRemQ, typename IIQL>
pud_node_deactivator<IGAS, IEAS, IRelQ, IRemQ, IIQL>::pud_node_deactivator(
    IGAS& get_active_state,
    IEAS& erase_active_state,
    IRelQ& release_query,
    IRemQ& remove_query,
    IIQL& invalidate_query_leaf)
    : get_active_state_(get_active_state)
    , erase_active_state_(erase_active_state)
    , release_query_(release_query)
    , remove_query_(remove_query)
    , invalidate_query_leaf_(invalidate_query_leaf) {}

template<typename IGAS, typename IEAS, typename IRelQ, typename IRemQ, typename IIQL>
std::vector<pud_mhq_head_id> pud_node_deactivator<IGAS, IEAS, IRelQ, IRemQ, IIQL>::deactivate(pud_node_id node) {
    for (const auto& [goal, head_id] : get_active_state_.get(node).queries) {
        if (!release_query_.release(head_id, node))
            continue;
        remove_query_.remove(head_id);
    }
    erase_active_state_.erase(node);
    return invalidate_query_leaf_.invalidate_leaf(node);
}

#endif
