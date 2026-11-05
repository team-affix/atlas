#ifndef PUD_STATE_AXIOMS_HPP
#define PUD_STATE_AXIOMS_HPP

#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_node_state.hpp"

// Reads a node's axiom: its root axiom, from its state.
template<typename IGetNodeState>
struct pud_state_axioms {
    pud_state_axioms(IGetNodeState& get_node_state);
    const pud_global_node_id& get(pud_global_node_id node) const;
private:
    IGetNodeState& get_node_state_;
};

template<typename IGetNodeState>
pud_state_axioms<IGetNodeState>::pud_state_axioms(IGetNodeState& get_node_state)
    : get_node_state_(get_node_state) {}

template<typename IGetNodeState>
const pud_global_node_id& pud_state_axioms<IGetNodeState>::get(pud_global_node_id node) const {
    return get_node_state_.get(node).axiom;
}

#endif
