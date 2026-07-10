#ifndef AVOIDANCE_BOUNDARY_FRAME_ASSIGN_HPP
#define AVOIDANCE_BOUNDARY_FRAME_ASSIGN_HPP

#include <compare>
#include <cstddef>

enum class avoidance_frame_slot : bool {
    ultimate_frame_index,
    unit_boundary_frame_index
};

struct avoidance_boundary_frame_assign {
    avoidance_frame_slot slot;
    size_t previous;
    auto operator<=>(const avoidance_boundary_frame_assign&) const = default;
};

#endif
