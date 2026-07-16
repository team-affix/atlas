#ifndef VECTOR_POP_BACK_GOAL_HPP
#define VECTOR_POP_BACK_GOAL_HPP

#include <compare>
#include "value_objects/lineage.hpp"

struct vector_pop_back_goal {
    const goal_lineage* gl;
    auto operator<=>(const vector_pop_back_goal&) const = default;
};

#endif
