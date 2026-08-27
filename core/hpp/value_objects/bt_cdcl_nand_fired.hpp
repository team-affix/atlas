#ifndef BT_CDCL_NAND_FIRED_HPP
#define BT_CDCL_NAND_FIRED_HPP

#include <compare>
#include "value_objects/bt_cdcl_factor.hpp"

struct bt_cdcl_nand_fired {
    bt_cdcl_factor* node;
    auto operator<=>(const bt_cdcl_nand_fired&) const = default;
};

#endif
