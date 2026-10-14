#ifndef PUD_REPLAY_DESCENT_HPP
#define PUD_REPLAY_DESCENT_HPP

#include <cstdint>
#include "value_objects/pud_ubs_state.hpp"

struct pud_replay_descent {
    uint32_t frame_offset;
    pud_ubs_state ubs_state;
    auto operator<=>(const pud_replay_descent&) const = default;
};

#endif
