#ifndef GOAL_DEPTH_ACTION_HPP
#define GOAL_DEPTH_ACTION_HPP

#include <variant>
#include "value_objects/goal_depth_erase.hpp"
#include "value_objects/goal_depth_insert.hpp"

using goal_depth_action = std::variant<goal_depth_insert, goal_depth_erase>;

#endif
