#ifndef AVOIDANCE_MADE_UNEVICTABLE_HPP
#define AVOIDANCE_MADE_UNEVICTABLE_HPP

#include <compare>
#include "value_objects/avoidance_id.hpp"

struct avoidance_made_unevictable {
    avoidance_id id;
    auto operator<=>(const avoidance_made_unevictable&) const = default;
};

#endif
