#ifndef I_WEIGHT_FRONTIER_HPP
#define I_WEIGHT_FRONTIER_HPP

#include "../value_objects/lineage.hpp"
#include "i_map.hpp"

struct i_weight_frontier : i_map<const goal_lineage*, double> {
    virtual ~i_weight_frontier() = default;
};

#endif
