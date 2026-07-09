#ifndef RAISED_UNIT_AVOIDANCE_ACTION_HPP
#define RAISED_UNIT_AVOIDANCE_ACTION_HPP

#include <compare>
#include "value_objects/avoidance_id.hpp"

struct raised_unit_avoidance {
    avoidance_id id;
    size_t unit_boundary;
    auto operator<=>(const raised_unit_avoidance&) const = default;
};

#endif
