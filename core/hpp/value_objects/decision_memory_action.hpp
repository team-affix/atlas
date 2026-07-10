#ifndef DECISION_MEMORY_ACTION_HPP
#define DECISION_MEMORY_ACTION_HPP

#include <variant>
#include "value_objects/resolution_set_insert.hpp"

using decision_memory_action = std::variant<resolution_set_insert>;

#endif
