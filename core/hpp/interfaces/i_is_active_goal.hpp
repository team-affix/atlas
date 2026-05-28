#ifndef I_IS_ACTIVE_GOAL_HPP
#define I_IS_ACTIVE_GOAL_HPP

#include "value_objects/lineage.hpp"

struct i_is_active_goal {
    virtual ~i_is_active_goal() = default;
    virtual bool is_active_goal(const goal_lineage*) const = 0;
};

#endif
