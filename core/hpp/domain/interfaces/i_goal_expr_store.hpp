#ifndef I_GOAL_EXPR_STORE_HPP
#define I_GOAL_EXPR_STORE_HPP

#include "../value_objects/expr.hpp"
#include "../value_objects/lineage.hpp"
#include "i_map.hpp"

struct i_goal_expr_store : i_map<const goal_lineage*, const expr*> {
    virtual ~i_goal_expr_store() = default;
};

#endif
