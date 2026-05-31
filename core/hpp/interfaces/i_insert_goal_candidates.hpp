#ifndef I_INSERT_GOAL_CANDIDATES_HPP
#define I_INSERT_GOAL_CANDIDATES_HPP

#include "value_objects/lineage.hpp"

struct i_insert_goal_candidates {
    virtual ~i_insert_goal_candidates() = default;
    virtual void insert(const goal_lineage*) = 0;
};

#endif
