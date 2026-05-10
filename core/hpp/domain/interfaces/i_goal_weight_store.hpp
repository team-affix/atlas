#ifndef I_GOAL_WEIGHT_STORE_HPP
#define I_GOAL_WEIGHT_STORE_HPP

#include "../value_objects/lineage.hpp"
#include "i_map.hpp"

struct i_goal_weight_store : i_map<const goal_lineage*, double> {
    virtual ~i_goal_weight_store() = default;
};

#endif
