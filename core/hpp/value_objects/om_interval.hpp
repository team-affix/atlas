#ifndef OM_INTERVAL_HPP
#define OM_INTERVAL_HPP

#include <compare>
#include "value_objects/om_label.hpp"

struct om_interval {
    om_label open;
    om_label close;
    std::strong_ordering operator<=>(const om_interval& other) const;
    bool operator==(const om_interval& other) const;
};

#endif
