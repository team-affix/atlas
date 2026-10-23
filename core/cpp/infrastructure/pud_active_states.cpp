#include "infrastructure/pud_active_states.hpp"
#include <utility>
#include "debug_assert.hpp"

pud_active_states::pud_active_states()
    : states_() {}

void pud_active_states::store(pud_global_node_id id, pud_active_state state) {
    [[maybe_unused]] const bool stored = states_.emplace(id, std::move(state)).second;
    DEBUG_ASSERT(stored);
}

const pud_active_state& pud_active_states::get(pud_global_node_id id) const {
    return states_.at(id);
}

void pud_active_states::erase(pud_global_node_id id) {
    [[maybe_unused]] const size_t erased = states_.erase(id);
    DEBUG_ASSERT(erased == 1);
}
