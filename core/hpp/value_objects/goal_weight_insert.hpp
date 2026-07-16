#ifndef GOAL_WEIGHT_INSERT_HPP
#define GOAL_WEIGHT_INSERT_HPP

#include <compare>
#include "value_objects/lineage.hpp"

struct goal_weight_insert {
    const goal_lineage* gl;
    auto operator<=>(const goal_weight_insert&) const = default;
};

#endif
