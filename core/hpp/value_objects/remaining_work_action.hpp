#ifndef REMAINING_WORK_ACTION_HPP
#define REMAINING_WORK_ACTION_HPP

#include <variant>
#include "value_objects/scalar_add_f64.hpp"

using remaining_work_action = std::variant<scalar_add_f64>;

#endif
