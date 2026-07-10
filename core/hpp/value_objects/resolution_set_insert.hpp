#ifndef RESOLUTION_SET_INSERT_HPP
#define RESOLUTION_SET_INSERT_HPP

#include <compare>
#include "value_objects/lineage.hpp"

struct resolution_set_insert {
    const resolution_lineage* rl;
    auto operator<=>(const resolution_set_insert&) const = default;
};

#endif
