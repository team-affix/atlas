#ifndef I_UNSET_GOAL_EXPR_HPP
#define I_UNSET_GOAL_EXPR_HPP

#include "../value_objects/lineage.hpp"

struct i_unset_goal_expr {
    virtual ~i_unset_goal_expr() = default;
    virtual void unset(const goal_lineage*) = 0;
};

#endif
