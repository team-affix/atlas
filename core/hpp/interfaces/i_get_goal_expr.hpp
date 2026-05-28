#ifndef I_GET_GOAL_EXPR_HPP
#define I_GET_GOAL_EXPR_HPP

#include "value_objects/lineage.hpp"

struct i_get_goal_expr {
    virtual ~i_get_goal_expr() = default;
    virtual const expr* get(const goal_lineage*) const = 0;
};

#endif
