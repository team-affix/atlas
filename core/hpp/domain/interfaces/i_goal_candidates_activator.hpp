#ifndef I_GOAL_CANDIDATES_ACTIVATOR_HPP
#define I_GOAL_CANDIDATES_ACTIVATOR_HPP

#include "../value_objects/lineage.hpp"

struct i_goal_candidates_activator {
    virtual ~i_goal_candidates_activator() = default;
    virtual void init_activate(const goal_lineage*) = 0;
    virtual void resume() = 0;
};

#endif
