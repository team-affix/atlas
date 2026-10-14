#ifndef PUD_BODY_GOAL_EXPR_DESCENT_HPP
#define PUD_BODY_GOAL_EXPR_DESCENT_HPP

#include <immer/map.hpp>
#include "value_objects/body_goal_id.hpp"
#include "value_objects/expr.hpp"

struct pud_body_goal_expr_descent {
    immer::map<body_goal_id, const expr*> body_goal_exprs;
    body_goal_id next_body_goal_id;
    auto operator<=>(const pud_body_goal_expr_descent&) const = default;
};

#endif
