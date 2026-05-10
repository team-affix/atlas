#ifndef I_EXPR_FRONTIER_HPP
#define I_EXPR_FRONTIER_HPP

#include "../value_objects/expr.hpp"
#include "../value_objects/lineage.hpp"
#include "i_map.hpp"

struct i_expr_frontier : i_map<const goal_lineage*, const expr*> {
    virtual ~i_expr_frontier() = default;
};

#endif
