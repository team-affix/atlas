#ifndef CUMULATIVE_GROUNDED_WEIGHT_ACTION_HPP
#define CUMULATIVE_GROUNDED_WEIGHT_ACTION_HPP

#include <variant>
#include "value_objects/scalar_add_f64.hpp"

using cumulative_grounded_weight_action = std::variant<scalar_add_f64>;

#endif
