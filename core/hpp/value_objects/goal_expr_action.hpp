#ifndef GOAL_EXPR_ACTION_HPP
#define GOAL_EXPR_ACTION_HPP

#include <variant>
#include "value_objects/goal_expr_insert.hpp"
#include "value_objects/goal_expr_erase.hpp"

using goal_expr_action = std::variant<goal_expr_insert, goal_expr_erase>;

#endif
