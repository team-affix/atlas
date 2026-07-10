#ifndef GOAL_EXPR_INSERT_HPP
#define GOAL_EXPR_INSERT_HPP

#include <compare>
#include "value_objects/framed_expr.hpp"
#include "value_objects/lineage.hpp"

struct goal_expr_insert {
    const goal_lineage* gl;
    framed_expr value;
    auto operator<=>(const goal_expr_insert&) const = default;
};

#endif
