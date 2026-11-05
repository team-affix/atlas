#ifndef PUD_STATE_DEPTHS_HPP
#define PUD_STATE_DEPTHS_HPP

#include <cstddef>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_node_state.hpp"

// Reads a node's depth: its depth below its root axiom, from its state.
template<typename IGetNodeState>
struct pud_state_depths {
    pud_state_depths(IGetNodeState& get_node_state);
    const size_t& get(pud_global_node_id node) const;
private:
    IGetNodeState& get_node_state_;
};

template<typename IGetNodeState>
pud_state_depths<IGetNodeState>::pud_state_depths(IGetNodeState& get_node_state)
    : get_node_state_(get_node_state) {}

template<typename IGetNodeState>
const size_t& pud_state_depths<IGetNodeState>::get(pud_global_node_id node) const {
    return get_node_state_.get(node).depth;
}

#endif
