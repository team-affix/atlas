#ifndef PUD_BASE_DESCENT_HPP
#define PUD_BASE_DESCENT_HPP

#include "value_objects/pud_binding_descent.hpp"
#include "value_objects/pud_body_goal_expr_descent.hpp"
#include "value_objects/pud_body_goal_pq_descent.hpp"
#include "value_objects/pud_lvc_descent.hpp"
#include "value_objects/pud_next_body_goal_id_descent.hpp"

struct pud_base_descent {
    pud_binding_descent binding;
    pud_lvc_descent lvc;
    pud_next_body_goal_id_descent next_body_goal_id;
    pud_body_goal_expr_descent body_goal_expr;
    pud_body_goal_pq_descent body_goal_pq;
    auto operator<=>(const pud_base_descent&) const = default;
};

#endif
