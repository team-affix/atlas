#ifndef PUD_ACTIVE_STATES_HPP
#define PUD_ACTIVE_STATES_HPP

#include <unordered_map>
#include "value_objects/pud_active_state.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_global_node_id_hash.hpp"

// Active state stored per node while it is live on the frontier; erased when the node is unfolded or refuted.
struct pud_active_states {
    pud_active_states();
    void store(pud_global_node_id id, pud_active_state state);
    const pud_active_state& get(pud_global_node_id id) const;
    void erase(pud_global_node_id id);
private:
    std::unordered_map<pud_global_node_id, pud_active_state, pud_global_node_id_hash> states_;
};

#endif
