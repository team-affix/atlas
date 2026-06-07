#ifndef I_ACTIVATE_GOAL_CANDIDATES_HPP
#define I_ACTIVATE_GOAL_CANDIDATES_HPP

#include "value_objects/lineage.hpp"

struct i_activate_goal_candidates {
    virtual ~i_activate_goal_candidates() = default;
    virtual bool activate_goal_candidates(const goal_lineage*) = 0;
};

#endif
