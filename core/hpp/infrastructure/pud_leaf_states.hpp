#ifndef PUD_LEAF_STATES_HPP
#define PUD_LEAF_STATES_HPP

#include <deque>
#include <optional>
#include "value_objects/pud_leaf_state.hpp"
#include "value_objects/pud_node_id.hpp"

// The state of every live leaf: stored when the leaf is activated, erased when it is expanded or
// refuted.
struct pud_leaf_states {
    pud_leaf_states();
    void store(pud_node_id id, pud_leaf_state state);
    const pud_leaf_state& get(pud_node_id id) const;
    void erase(pud_node_id id);
private:
    std::deque<std::optional<pud_leaf_state>> states_;
};

#endif
