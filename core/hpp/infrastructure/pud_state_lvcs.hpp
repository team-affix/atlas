#ifndef PUD_STATE_LVCS_HPP
#define PUD_STATE_LVCS_HPP

#include <cstdint>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_node_state.hpp"

// Reads a node's lvc: how many variables its path has introduced, from its state.
template<typename IGetNodeState>
struct pud_state_lvcs {
    pud_state_lvcs(IGetNodeState& get_node_state);
    const uint32_t& get(pud_global_node_id node) const;
private:
    IGetNodeState& get_node_state_;
};

template<typename IGetNodeState>
pud_state_lvcs<IGetNodeState>::pud_state_lvcs(IGetNodeState& get_node_state)
    : get_node_state_(get_node_state) {}

template<typename IGetNodeState>
const uint32_t& pud_state_lvcs<IGetNodeState>::get(pud_global_node_id node) const {
    return get_node_state_.get(node).lvc;
}

#endif
