#ifndef PUD_STATE_EVICTING_DEACTIVATOR_HPP
#define PUD_STATE_EVICTING_DEACTIVATOR_HPP

#include <vector>
#include "value_objects/pud_global_node_id.hpp"

// Deactivates a node whose only live field is its cached state: the state is dropped at once, since a refuted leaf is
// never read again. Nothing else depends on it, so it strands no leaf.
template<typename IEvictNodeState>
struct pud_state_evicting_deactivator {
    pud_state_evicting_deactivator(IEvictNodeState& evict_node_state);
    std::vector<pud_global_node_id> deactivate(pud_global_node_id node);
private:
    IEvictNodeState& evict_node_state_;
};

template<typename IEvictNodeState>
pud_state_evicting_deactivator<IEvictNodeState>::pud_state_evicting_deactivator(IEvictNodeState& evict_node_state)
    : evict_node_state_(evict_node_state) {}

template<typename IEvictNodeState>
std::vector<pud_global_node_id> pud_state_evicting_deactivator<IEvictNodeState>::deactivate(pud_global_node_id node) {
    evict_node_state_.evict(node);
    return {};
}

#endif
