#ifndef PUD_STATE_BINDINGS_HPP
#define PUD_STATE_BINDINGS_HPP

#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_ubs_state.hpp"
#include "value_objects/pud_node_state.hpp"

// Reads a node's bindings: its bindings, from its state.
template<typename IGetNodeState>
struct pud_state_bindings {
    pud_state_bindings(IGetNodeState& get_node_state);
    const pud_ubs_state& get(pud_global_node_id node) const;
private:
    IGetNodeState& get_node_state_;
};

template<typename IGetNodeState>
pud_state_bindings<IGetNodeState>::pud_state_bindings(IGetNodeState& get_node_state)
    : get_node_state_(get_node_state) {}

template<typename IGetNodeState>
const pud_ubs_state& pud_state_bindings<IGetNodeState>::get(pud_global_node_id node) const {
    return get_node_state_.get(node).bindings;
}

#endif
