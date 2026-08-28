#ifndef BT_CDCL_FACTOR_HPP
#define BT_CDCL_FACTOR_HPP

#include <compare>
#include <cstddef>
#include <vector>
#include "value_objects/lineage.hpp"

struct bt_cdcl_factor {
    bt_cdcl_factor(size_t tuple_size,
                   bt_cdcl_factor* left,
                   bt_cdcl_factor* right,
                   const resolution_lineage* leaf_rl);
    size_t tuple_size;
    bt_cdcl_factor* left;
    bt_cdcl_factor* right;
    const resolution_lineage* leaf_rl;
    std::vector<bt_cdcl_factor*> parents;
    size_t visited;
    size_t visited_generation;
    bool is_avoidance;
    size_t fired_generation;
    bool nand_fired;
    bool armed;
    auto operator<=>(const bt_cdcl_factor&) const = default;
};

#endif
