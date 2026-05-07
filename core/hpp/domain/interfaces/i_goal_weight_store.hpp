#ifndef I_GOAL_WEIGHT_STORE_HPP
#define I_GOAL_WEIGHT_STORE_HPP

#include "i_goal_store.hpp"

struct i_goal_weight_store : i_goal_store<double> {
    virtual ~i_goal_weight_store() = default;
};

#endif
