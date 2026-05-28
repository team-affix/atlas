#ifndef I_ACTIVATE_INITIAL_GOAL_HPP
#define I_ACTIVATE_INITIAL_GOAL_HPP

#include "value_objects/lineage.hpp"

struct i_activate_initial_goal {
    virtual ~i_activate_initial_goal() = default;
    virtual void activate_initial_goal(subgoal_id idx) = 0;
};

#endif
