#ifndef I_GET_INITIAL_GOAL_EXPR_HPP
#define I_GET_INITIAL_GOAL_EXPR_HPP

#include <cstddef>
#include "value_objects/expr.hpp"

struct i_get_initial_goal_expr {
    virtual ~i_get_initial_goal_expr() = default;
    virtual const expr* get(size_t idx) const = 0;
};

#endif
