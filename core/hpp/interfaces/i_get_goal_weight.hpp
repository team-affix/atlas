#ifndef I_GET_GOAL_WEIGHT_HPP
#define I_GET_GOAL_WEIGHT_HPP

#include "value_objects/lineage.hpp"

struct i_get_goal_weight {
    virtual ~i_get_goal_weight() = default;
    virtual double get(const goal_lineage*) const = 0;
};

#endif
