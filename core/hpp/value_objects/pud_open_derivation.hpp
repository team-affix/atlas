#ifndef PUD_OPEN_DERIVATION_HPP
#define PUD_OPEN_DERIVATION_HPP

#include <compare>
#include <vector>
#include "value_objects/pud_derivation_id.hpp"
#include "value_objects/pud_open_goal.hpp"

struct pud_open_derivation {
    pud_derivation_id derivation;
    std::vector<pud_open_goal> goals;
    auto operator<=>(const pud_open_derivation&) const = default;
};

#endif
