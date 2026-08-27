#ifndef BT_CDCL_PAIR_KEY_HPP
#define BT_CDCL_PAIR_KEY_HPP

#include <compare>
#include "value_objects/bt_cdcl_factor.hpp"

struct bt_cdcl_pair_key {
    bt_cdcl_factor* left;
    bt_cdcl_factor* right;
    auto operator<=>(const bt_cdcl_pair_key&) const = default;
};

#endif
