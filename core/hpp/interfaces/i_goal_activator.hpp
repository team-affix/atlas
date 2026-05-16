#ifndef I_GOAL_ACTIVATOR_HPP
#define I_GOAL_ACTIVATOR_HPP

#include "../value_objects/lineage.hpp"

struct i_goal_activator {
    virtual ~i_goal_activator() = default;
    virtual bool activate(const goal_lineage*) = 0;
};

#endif
