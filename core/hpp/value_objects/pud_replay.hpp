#ifndef PUD_REPLAY_HPP
#define PUD_REPLAY_HPP

#include <cstdint>
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_ubs_state.hpp"

// A query's view of one database node: the caller's environment extended by every unification on the
// way down to the node, with the callee's variables framed at frame_offset.
struct pud_replay {
    pud_node_id node;
    uint32_t frame_offset;
    pud_ubs_state ubs_state;
    bool operator==(const pud_replay&) const = default;
};

#endif
