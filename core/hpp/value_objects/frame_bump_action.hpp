#ifndef FRAME_BUMP_ACTION_HPP
#define FRAME_BUMP_ACTION_HPP

#include <variant>
#include "value_objects/scalar_add_u32.hpp"

using frame_bump_action = std::variant<scalar_add_u32>;

#endif
