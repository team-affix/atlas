#include "infrastructure/pud_leaf_states.hpp"
#include <utility>
#include "debug_assert.hpp"

pud_leaf_states::pud_leaf_states()
    : states_() {}

void pud_leaf_states::store(pud_node_id id, pud_leaf_state state) {
    if (states_.size() <= id)
        states_.resize(id + 1);
    DEBUG_ASSERT(!states_.at(id).has_value());
    states_.at(id) = std::move(state);
}

const pud_leaf_state& pud_leaf_states::get(pud_node_id id) const {
    return states_.at(id).value();
}

void pud_leaf_states::erase(pud_node_id id) {
    DEBUG_ASSERT(states_.at(id).has_value());
    states_.at(id) = std::nullopt;
}
