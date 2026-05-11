#ifndef I_GOAL_CANDIDATES_DEACTIVATOR_HPP
#define I_GOAL_CANDIDATES_DEACTIVATOR_HPP

#include "../value_objects/lineage.hpp"

struct i_goal_candidates_deactivator {
    virtual ~i_goal_candidates_deactivator() = default;
    virtual void deactivate(const goal_lineage*) = 0;
};

#endif
