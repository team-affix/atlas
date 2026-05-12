#ifndef I_GOAL_FACTORY_HPP
#define I_GOAL_FACTORY_HPP

#include "i_factory.hpp"
#include "i_goal.hpp"
#include "../value_objects/lineage.hpp"
#include "../value_objects/expr.hpp"

struct i_goal_factory : i_factory<i_goal, const goal_lineage*, const expr*> {
    virtual ~i_goal_factory() = default;
};

#endif
