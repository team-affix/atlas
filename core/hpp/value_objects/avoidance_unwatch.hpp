#ifndef AVOIDANCE_UNWATCH_HPP
#define AVOIDANCE_UNWATCH_HPP

#include <compare>
#include "value_objects/avoidance_id.hpp"

struct avoidance_unwatch {
    avoidance_id id;
    auto operator<=>(const avoidance_unwatch&) const = default;
};

#endif
