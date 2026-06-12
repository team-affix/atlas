#ifndef I_ERASE_GOAL_WEIGHT_HPP
#define I_ERASE_GOAL_WEIGHT_HPP

#include "value_objects/lineage.hpp"

struct i_erase_goal_weight {
    virtual ~i_erase_goal_weight() = default;
    virtual void erase(const goal_lineage*) = 0;
};

#endif
