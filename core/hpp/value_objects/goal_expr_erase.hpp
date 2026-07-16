#ifndef GOAL_EXPR_ERASE_HPP
#define GOAL_EXPR_ERASE_HPP

#include <compare>
#include "value_objects/framed_expr.hpp"
#include "value_objects/lineage.hpp"

struct goal_expr_erase {
    const goal_lineage* gl;
    framed_expr value;
    auto operator<=>(const goal_expr_erase&) const = default;
};

#endif
