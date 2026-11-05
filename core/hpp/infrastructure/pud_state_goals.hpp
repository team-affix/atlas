#ifndef PUD_STATE_GOALS_HPP
#define PUD_STATE_GOALS_HPP

#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "value_objects/pud_node_state.hpp"

// Reads a node's goals: its open goals, from its state.
template<typename IGetNodeState>
struct pud_state_goals {
    pud_state_goals(IGetNodeState& get_node_state);
    const pud_goal_list& get(pud_global_node_id node) const;
private:
    IGetNodeState& get_node_state_;
};

template<typename IGetNodeState>
pud_state_goals<IGetNodeState>::pud_state_goals(IGetNodeState& get_node_state)
    : get_node_state_(get_node_state) {}

template<typename IGetNodeState>
const pud_goal_list& pud_state_goals<IGetNodeState>::get(pud_global_node_id node) const {
    return get_node_state_.get(node).goals;
}

#endif
