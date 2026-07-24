#ifndef GOAL_WORK_VALUE_ACTION_HPP
#define GOAL_WORK_VALUE_ACTION_HPP

#include <variant>
#include "value_objects/goal_work_value_erase.hpp"
#include "value_objects/goal_work_value_insert.hpp"

using goal_work_value_action = std::variant<goal_work_value_insert, goal_work_value_erase>;

#endif
