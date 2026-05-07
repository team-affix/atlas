#ifndef I_INITIAL_GOAL_INITIALIZER_HPP
#define I_INITIAL_GOAL_INITIALIZER_HPP

#include "../value_objects/lineage.hpp"

struct i_initial_goal_initializer {
    virtual ~i_initial_goal_initializer() = default;
    virtual void initialize(const goal_lineage*) = 0;
};

#endif
