#ifndef PUD_UNFOLD_CHOICE_HPP
#define PUD_UNFOLD_CHOICE_HPP

#include <compare>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_derivation_id.hpp"

struct pud_unfold_choice {
    pud_derivation_id derivation;
    pud_body_goal_id goal;
    auto operator<=>(const pud_unfold_choice&) const = default;
};

#endif
