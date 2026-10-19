#ifndef PUD_OPEN_GOAL_HPP
#define PUD_OPEN_GOAL_HPP

#include <compare>
#include <cstddef>
#include "value_objects/pud_body_goal_id.hpp"

struct pud_open_goal {
    pud_body_goal_id goal;
    size_t candidate_count;
    auto operator<=>(const pud_open_goal&) const = default;
};

#endif
