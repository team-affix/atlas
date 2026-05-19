#ifndef I_SET_GOAL_EXPR_HPP
#define I_SET_GOAL_EXPR_HPP

#include "../value_objects/lineage.hpp"

struct i_set_goal_expr {
    virtual ~i_set_goal_expr() = default;
    virtual void set(const goal_lineage*, const expr*) = 0;
};

#endif
