#ifndef AVOIDANCE_BOUNDARY_RL_ASSIGN_HPP
#define AVOIDANCE_BOUNDARY_RL_ASSIGN_HPP

#include <compare>
#include "value_objects/lineage.hpp"

enum class avoidance_rl_slot : bool {
    ultimate,
    penultimate
};

struct avoidance_boundary_rl_assign {
    avoidance_rl_slot slot;
    const resolution_lineage* previous;
    auto operator<=>(const avoidance_boundary_rl_assign&) const = default;
};

#endif
