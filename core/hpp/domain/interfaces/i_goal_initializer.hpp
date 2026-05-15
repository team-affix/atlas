#ifndef I_GOAL_INITIALIZER_HPP
#define I_GOAL_INITIALIZER_HPP

#include "../value_objects/lineage.hpp"

struct i_goal_initializer {
    virtual ~i_goal_initializer() = default;
    virtual void seed_expansion(const resolution_lineage*) = 0;
    virtual void initialize(const goal_lineage*) = 0;
};

#endif
