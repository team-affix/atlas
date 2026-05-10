#ifndef I_GOAL_ACTIVATOR_HPP
#define I_GOAL_ACTIVATOR_HPP

#include "../value_objects/lineage.hpp"

struct i_goal_activator {
    virtual ~i_goal_activator() = default;
    virtual void start_resolution(const resolution_lineage*) = 0;
    virtual void activate(const goal_lineage*) = 0;
};

#endif
