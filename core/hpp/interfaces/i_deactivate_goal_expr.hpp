#ifndef I_DEACTIVATE_GOAL_EXPR_HPP
#define I_DEACTIVATE_GOAL_EXPR_HPP

#include "../value_objects/lineage.hpp"

struct i_deactivate_goal_expr {
    virtual ~i_deactivate_goal_expr() = default;
    virtual void deactivate(const goal_lineage*) = 0;
};

#endif
