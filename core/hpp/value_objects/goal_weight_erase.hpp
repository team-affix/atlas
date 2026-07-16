#ifndef GOAL_WEIGHT_ERASE_HPP
#define GOAL_WEIGHT_ERASE_HPP

#include <compare>
#include "value_objects/lineage.hpp"

struct goal_weight_erase {
    const goal_lineage* gl;
    double previous_weight;
    auto operator<=>(const goal_weight_erase&) const = default;
};

#endif
