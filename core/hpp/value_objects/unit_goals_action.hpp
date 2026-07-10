#ifndef UNIT_GOALS_ACTION_HPP
#define UNIT_GOALS_ACTION_HPP

#include <variant>
#include "value_objects/vector_push_back_goal.hpp"
#include "value_objects/vector_pop_back_goal.hpp"

using unit_goals_action = std::variant<vector_push_back_goal, vector_pop_back_goal>;

#endif
