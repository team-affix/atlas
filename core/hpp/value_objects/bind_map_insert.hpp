#ifndef BIND_MAP_INSERT_HPP
#define BIND_MAP_INSERT_HPP

#include <compare>
#include <cstdint>
#include "value_objects/framed_expr.hpp"

struct bind_map_insert {
    uint32_t key;
    framed_expr value;
    auto operator<=>(const bind_map_insert&) const = default;
};

#endif
