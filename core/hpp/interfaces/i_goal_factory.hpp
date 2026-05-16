#ifndef I_GOAL_FACTORY_HPP
#define I_GOAL_FACTORY_HPP

#include "../interfaces/i_factory.hpp"
#include "../value_objects/goal.hpp"

struct i_goal_factory : i_factory<goal> {
    virtual ~i_goal_factory() = default;
};

#endif
