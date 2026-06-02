#ifndef I_PUSH_INITIAL_GOAL_EXPR_HPP
#define I_PUSH_INITIAL_GOAL_EXPR_HPP

#include "value_objects/expr.hpp"

struct i_push_initial_goal_expr {
    virtual ~i_push_initial_goal_expr() = default;
    virtual void push(const expr*) = 0;
};

#endif
