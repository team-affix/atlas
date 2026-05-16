#ifndef I_GOAL_DEACTIVATOR_HPP
#define I_GOAL_DEACTIVATOR_HPP

#include "../value_objects/lineage.hpp"

struct i_goal_deactivator {
    virtual ~i_goal_deactivator() = default;
    virtual bool deactivate(const goal_lineage*) = 0;
};

#endif
