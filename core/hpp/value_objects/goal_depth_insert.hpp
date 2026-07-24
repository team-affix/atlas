#ifndef GOAL_DEPTH_INSERT_HPP
#define GOAL_DEPTH_INSERT_HPP

#include <compare>
#include "value_objects/lineage.hpp"

struct goal_depth_insert {
    const goal_lineage* gl;
    auto operator<=>(const goal_depth_insert&) const = default;
};

#endif
