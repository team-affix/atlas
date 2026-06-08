#ifndef I_DEACTIVATE_GOAL_CANDIDATES_HPP
#define I_DEACTIVATE_GOAL_CANDIDATES_HPP

#include "value_objects/lineage.hpp"

struct i_deactivate_goal_candidates {
    virtual ~i_deactivate_goal_candidates() = default;
    virtual void deactivate_goal_candidates(const goal_lineage*) = 0;
};

#endif
