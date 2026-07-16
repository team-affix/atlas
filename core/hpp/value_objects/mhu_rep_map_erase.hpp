#ifndef MHU_REP_MAP_ERASE_HPP
#define MHU_REP_MAP_ERASE_HPP

#include <compare>
#include <cstdint>
#include <unordered_set>
#include "value_objects/lineage.hpp"

struct mhu_rep_map_erase {
    uint32_t rep;
    std::unordered_set<const resolution_lineage*> value;
    auto operator<=>(const mhu_rep_map_erase&) const = default;
};

#endif
