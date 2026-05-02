#ifndef I_GOAL_EXPR_STORE_HPP
#define I_GOAL_EXPR_STORE_HPP

#include "../value_objects/expr.hpp"
#include "../value_objects/lineage.hpp"

struct i_goal_expr_store {
    virtual ~i_goal_expr_store() = default;
    virtual void insert(const goal_lineage*, const expr*) = 0;
    virtual void erase(const goal_lineage*) = 0;
    virtual void clear() = 0;
    virtual const expr* at(const goal_lineage*) const = 0;
};

#endif
