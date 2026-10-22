#ifndef PUD_CANDIDATE_CHOICE_POINT_HPP
#define PUD_CANDIDATE_CHOICE_POINT_HPP

#include <compare>
#include "value_objects/pud_mhws_head_id.hpp"

struct pud_candidate_choice_point {
    pud_mhws_head_id witness_a;
    pud_mhws_head_id witness_b;
    auto operator<=>(const pud_candidate_choice_point&) const = default;
};

#endif
