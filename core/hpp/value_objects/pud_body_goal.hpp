#ifndef PUD_BODY_GOAL_HPP
#define PUD_BODY_GOAL_HPP

#include <compare>
#include "value_objects/expr.hpp"
#include "value_objects/pud_body_goal_id.hpp"

struct pud_body_goal {
    pud_body_goal_id id;
    const expr* goal;
    auto operator<=>(const pud_body_goal&) const = default;
};

#endif
