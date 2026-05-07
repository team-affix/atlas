#ifndef I_GOAL_EXPR_STORE_HPP
#define I_GOAL_EXPR_STORE_HPP

#include "../value_objects/expr.hpp"
#include "i_goal_store.hpp"

struct i_goal_expr_store : i_goal_store<const expr*> {
    virtual ~i_goal_expr_store() = default;
};

#endif
