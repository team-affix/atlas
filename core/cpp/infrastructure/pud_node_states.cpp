#include "infrastructure/pud_node_states.hpp"
#include "debug_assert.hpp"

pud_node_states::pud_node_states()
    : states_() {}

// A node is marked a leaf exactly once, when it is made; only a leaf is expanded or refuted.
void pud_node_states::mark(pud_node_id id, pud_node_state state) {
    if (state == pud_node_state::leaf) {
        DEBUG_ASSERT(id == states_.size());
        states_.push_back(state);
        return;
    }
    DEBUG_ASSERT(states_.at(id) == pud_node_state::leaf);
    states_.at(id) = state;
}

pud_node_state pud_node_states::get(pud_node_id id) const {
    return states_.at(id);
}
