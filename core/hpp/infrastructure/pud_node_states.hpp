#ifndef PUD_NODE_STATES_HPP
#define PUD_NODE_STATES_HPP

#include <deque>
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_node_state.hpp"

// The state of every node: a leaf from when it is made, until it is expanded or refuted.
struct pud_node_states {
    pud_node_states();
    void mark(pud_node_id id, pud_node_state state);
    pud_node_state get(pud_node_id id) const;
private:
    std::deque<pud_node_state> states_;
};

#endif
