#ifndef I_SET_GOAL_WEIGHT_HPP
#define I_SET_GOAL_WEIGHT_HPP

#include "value_objects/lineage.hpp"

struct i_set_goal_weight {
    virtual ~i_set_goal_weight() = default;
    virtual void set(const goal_lineage*, double) = 0;
};

#endif
