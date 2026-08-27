#ifndef RAISED_NAND_HPP
#define RAISED_NAND_HPP

#include <compare>
#include <cstddef>
#include "value_objects/bt_cdcl_factor.hpp"
#include "value_objects/lineage.hpp"

struct raised_nand {
    bt_cdcl_factor* nand;
    size_t unit_boundary;
    const resolution_lineage* ultimate;
    auto operator<=>(const raised_nand&) const = default;
};

#endif
