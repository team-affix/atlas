#ifndef BIND_MAP_ASSIGN_HPP
#define BIND_MAP_ASSIGN_HPP

#include <compare>
#include <cstdint>
#include "value_objects/framed_expr.hpp"

struct bind_map_assign {
    uint32_t key;
    framed_expr value;
    auto operator<=>(const bind_map_assign&) const = default;
};

#endif
