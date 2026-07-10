#ifndef AVOIDANCE_BOUNDARY_ACTION_HPP
#define AVOIDANCE_BOUNDARY_ACTION_HPP

#include <variant>
#include "value_objects/avoidance_boundary_frame_assign.hpp"
#include "value_objects/avoidance_boundary_rl_assign.hpp"

using avoidance_boundary_action = std::variant<
    avoidance_boundary_rl_assign,
    avoidance_boundary_frame_assign>;

#endif
