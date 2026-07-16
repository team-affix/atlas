#ifndef GOAL_WEIGHT_ACTION_HPP
#define GOAL_WEIGHT_ACTION_HPP

#include <variant>
#include "value_objects/goal_weight_erase.hpp"
#include "value_objects/goal_weight_insert.hpp"

using goal_weight_action = std::variant<goal_weight_insert, goal_weight_erase>;

#endif
