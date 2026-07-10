#ifndef MHU_RL_MAP_ERASE_HPP
#define MHU_RL_MAP_ERASE_HPP

#include <compare>
#include <cstdint>
#include <unordered_set>
#include "value_objects/lineage.hpp"

struct mhu_rl_map_erase {
    const resolution_lineage* rl;
    std::unordered_set<uint32_t> value;
    auto operator<=>(const mhu_rl_map_erase&) const = default;
};

#endif
