#ifndef PUD_CANDIDATE_CHOICE_POINT_HASH_HPP
#define PUD_CANDIDATE_CHOICE_POINT_HASH_HPP

#include <cstddef>
#include "value_objects/pud_candidate_choice_point.hpp"

struct pud_candidate_choice_point_hash {
    size_t operator()(const pud_candidate_choice_point& context) const noexcept;
};

#endif
