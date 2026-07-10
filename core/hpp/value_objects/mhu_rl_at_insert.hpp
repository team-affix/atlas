#ifndef MHU_RL_AT_INSERT_HPP
#define MHU_RL_AT_INSERT_HPP

#include <compare>
#include <cstdint>
#include "value_objects/lineage.hpp"

struct mhu_rl_at_insert {
    const resolution_lineage* rl;
    uint32_t rep;
    auto operator<=>(const mhu_rl_at_insert&) const = default;
};

#endif
