#ifndef PUD_REPLAY_HPP
#define PUD_REPLAY_HPP

#include <cstdint>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_ubs_state.hpp"

struct pud_replay {
    pud_global_node_id node;
    uint32_t frame_offset;
    pud_ubs_state ubs_state;
    bool operator==(const pud_replay&) const = default;
};

#endif
