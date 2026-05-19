#ifndef I_ACTIVATE_GOAL_EXPR_HPP
#define I_ACTIVATE_GOAL_EXPR_HPP

#include "../value_objects/lineage.hpp"

struct i_activate_goal_expr {
    virtual ~i_activate_goal_expr() = default;
    virtual void activate(const goal_lineage*, const expr*) = 0;
};

#endif
