#ifndef GOAL_DEPTH_ERASE_HPP
#define GOAL_DEPTH_ERASE_HPP

#include <compare>
#include <cstddef>
#include "value_objects/lineage.hpp"

struct goal_depth_erase {
    const goal_lineage* gl;
    size_t previous_depth;
    auto operator<=>(const goal_depth_erase&) const = default;
};

#endif
