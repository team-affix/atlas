#ifndef PUD_CANDIDATE_JUSTIFICATION_HPP
#define PUD_CANDIDATE_JUSTIFICATION_HPP

#include <variant>
#include "value_objects/pud_candidate_choice_point.hpp"
#include "value_objects/pud_candidate_self_witness.hpp"

using pud_candidate_justification = std::variant<
    pud_candidate_self_witness,
    pud_candidate_choice_point>;

#endif
