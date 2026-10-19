#ifndef PUD_UNIT_GOAL_HPP
#define PUD_UNIT_GOAL_HPP

#include <compare>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_derivation_id.hpp"

struct pud_unit_goal {
    pud_derivation_id derivation;
    pud_body_goal_id goal;
    auto operator<=>(const pud_unit_goal&) const = default;
};

#endif
