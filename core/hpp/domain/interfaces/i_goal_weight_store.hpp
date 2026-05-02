#ifndef I_GOAL_WEIGHT_STORE_HPP
#define I_GOAL_WEIGHT_STORE_HPP

#include "../value_objects/lineage.hpp"

struct i_goal_weight_store {
    virtual ~i_goal_weight_store() = default;
    virtual void insert(const goal_lineage*, double) = 0;
    virtual void erase(const goal_lineage*) = 0;
    virtual void clear() = 0;
    virtual double at(const goal_lineage*) const = 0;
};

#endif
