#ifndef MHU_RL_AT_ERASE_HPP
#define MHU_RL_AT_ERASE_HPP

#include <compare>
#include <cstdint>
#include "value_objects/lineage.hpp"

struct mhu_rl_at_erase {
    const resolution_lineage* rl;
    uint32_t rep;
    auto operator<=>(const mhu_rl_at_erase&) const = default;
};

#endif
