#ifndef NEAREST_DECISION_ACTION_HPP
#define NEAREST_DECISION_ACTION_HPP

#include <variant>
#include "value_objects/nearest_decision_insert.hpp"

using nearest_decision_action = std::variant<nearest_decision_insert>;

#endif
