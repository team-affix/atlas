#ifndef VECTOR_PUSH_BACK_GOAL_HPP
#define VECTOR_PUSH_BACK_GOAL_HPP

#include <compare>
#include "value_objects/lineage.hpp"

struct vector_push_back_goal {
    const goal_lineage* gl;
    auto operator<=>(const vector_push_back_goal&) const = default;
};

#endif
