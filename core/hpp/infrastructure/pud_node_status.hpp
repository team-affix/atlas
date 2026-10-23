#ifndef PUD_NODE_STATUS_HPP
#define PUD_NODE_STATUS_HPP

#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_node_state.hpp"

// A node is refuted when it is a refuted leaf, or an interior node whose children are all refuted.
template<typename IGetNodeState, typename IGetLiveChildCount>
struct pud_node_status {
    pud_node_status(IGetNodeState& get_node_state, IGetLiveChildCount& get_live_child_count);
    bool check_leaf(pud_node_id node) const;
    bool check_refuted(pud_node_id node) const;
private:
    IGetNodeState& get_node_state_;
    IGetLiveChildCount& get_live_child_count_;
};

template<typename IGNS, typename IGLCC>
pud_node_status<IGNS, IGLCC>::pud_node_status(IGNS& get_node_state, IGLCC& get_live_child_count)
    : get_node_state_(get_node_state)
    , get_live_child_count_(get_live_child_count) {}

template<typename IGNS, typename IGLCC>
bool pud_node_status<IGNS, IGLCC>::check_leaf(pud_node_id node) const {
    return get_node_state_.get(node) != pud_node_state::expanded;
}

template<typename IGNS, typename IGLCC>
bool pud_node_status<IGNS, IGLCC>::check_refuted(pud_node_id node) const {
    const pud_node_state state = get_node_state_.get(node);
    if (state == pud_node_state::expanded)
        return get_live_child_count_.get(node) == 0;
    return state == pud_node_state::refuted;
}

#endif
