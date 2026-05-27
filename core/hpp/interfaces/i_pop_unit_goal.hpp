#ifndef I_POP_UNIT_GOAL_HPP
#define I_POP_UNIT_GOAL_HPP

#include "../value_objects/lineage.hpp"

struct i_pop_unit_goal {
    virtual ~i_pop_unit_goal() = default;
    virtual const goal_lineage* pop() = 0;
};

#endif
