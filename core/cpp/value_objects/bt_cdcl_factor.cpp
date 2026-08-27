#include "value_objects/bt_cdcl_factor.hpp"

bt_cdcl_factor::bt_cdcl_factor(
    size_t tuple_size,
    bt_cdcl_factor* left,
    bt_cdcl_factor* right,
    const resolution_lineage* leaf_rl)
    : tuple_size(tuple_size)
    , left(left)
    , right(right)
    , leaf_rl(leaf_rl)
    , parents()
    , visited(0)
    , visited_generation(0)
    , nand_multiplicity(0)
    , fired_generation(0)
    , nand_fired(false)
    , armed(false) {}
