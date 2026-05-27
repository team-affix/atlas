#ifndef I_INSERT_ACTIVE_GOAL_HPP
#define I_INSERT_ACTIVE_GOAL_HPP

#include "../value_objects/lineage.hpp"

struct i_insert_active_goal {
    virtual ~i_insert_active_goal() = default;
    virtual void insert_active_goal(const goal_lineage*) = 0;
};

#endif
