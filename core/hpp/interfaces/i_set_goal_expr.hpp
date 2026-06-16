#ifndef I_SET_GOAL_EXPR_HPP
#define I_SET_GOAL_EXPR_HPP

#include "value_objects/lineage.hpp"
#include "value_objects/framed_expr.hpp"

struct i_set_goal_expr {
    virtual ~i_set_goal_expr() = default;
    virtual void set(const goal_lineage*, framed_expr) = 0;
};

#endif
