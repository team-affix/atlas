#ifndef BT_CDCL_VISIT_DELTA_HPP
#define BT_CDCL_VISIT_DELTA_HPP

#include <compare>
#include <cstddef>
#include "value_objects/bt_cdcl_factor.hpp"

struct bt_cdcl_visit_delta {
    bt_cdcl_factor* node;
    size_t amount;
    auto operator<=>(const bt_cdcl_visit_delta&) const = default;
};

#endif
