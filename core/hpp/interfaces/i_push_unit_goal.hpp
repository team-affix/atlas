#ifndef I_PUSH_UNIT_GOAL_HPP
#define I_PUSH_UNIT_GOAL_HPP

#include "../value_objects/lineage.hpp"

struct i_push_unit_goal {
    virtual ~i_push_unit_goal() = default;
    virtual void push(const goal_lineage*) = 0;
};

#endif
