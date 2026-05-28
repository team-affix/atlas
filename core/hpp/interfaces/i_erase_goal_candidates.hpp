#ifndef I_ERASE_GOAL_CANDIDATES_HPP
#define I_ERASE_GOAL_CANDIDATES_HPP

#include "value_objects/lineage.hpp"

struct i_erase_goal_candidates {
    virtual ~i_erase_goal_candidates() = default;
    virtual void erase(const goal_lineage*) = 0;
};

#endif
