#ifndef I_GOAL_CANDIDATES_DEACTIVATOR_HPP
#define I_GOAL_CANDIDATES_DEACTIVATOR_HPP

#include "../value_objects/lineage.hpp"

struct i_goal_candidates_deactivator {
    virtual ~i_goal_candidates_deactivator() = default;
    virtual void init_deactivate(const goal_lineage*) = 0;
    virtual void resume() = 0;
};

#endif
