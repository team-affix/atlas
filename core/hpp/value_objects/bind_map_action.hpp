#ifndef BIND_MAP_ACTION_HPP
#define BIND_MAP_ACTION_HPP

#include <variant>
#include "value_objects/bind_map_insert.hpp"
#include "value_objects/bind_map_assign.hpp"

using bind_map_action = std::variant<bind_map_insert, bind_map_assign>;

#endif
