#ifndef PUD_NODE_STATE_HPP
#define PUD_NODE_STATE_HPP

#include <cstddef>
#include <cstdint>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "value_objects/pud_ubs_state.hpp"

// What a node's path from its root axiom works out to: its open goals, its root axiom, its depth below that root,
// how many variables its path has introduced, and its bindings.
struct pud_node_state {
    pud_goal_list goals;
    pud_global_node_id axiom;
    size_t depth;
    uint32_t lvc;
    pud_ubs_state bindings;
    bool operator==(const pud_node_state&) const = default;
};

#endif
