#ifndef CANDIDATE_OFFSET_ERASE_HPP
#define CANDIDATE_OFFSET_ERASE_HPP

#include <compare>
#include <cstdint>
#include "value_objects/lineage.hpp"

struct candidate_offset_erase {
    const resolution_lineage* rl;
    uint32_t frame_offset;
    auto operator<=>(const candidate_offset_erase&) const = default;
};

#endif
