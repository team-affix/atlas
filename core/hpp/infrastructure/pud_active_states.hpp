#ifndef PUD_ACTIVE_STATES_HPP
#define PUD_ACTIVE_STATES_HPP

#include <deque>
#include <optional>
#include "value_objects/pud_active_state.hpp"
#include "value_objects/pud_node_id.hpp"

// Active state stored per node while it is live on the frontier; erased when the node is expanded or refuted.
struct pud_active_states {
    pud_active_states();
    void store(pud_node_id id, pud_active_state state);
    const pud_active_state& get(pud_node_id id) const;
    void erase(pud_node_id id);
private:
    std::deque<std::optional<pud_active_state>> states_;
};

#endif
