#ifndef PUD_SOLUTION_HPP
#define PUD_SOLUTION_HPP

#include <compare>
#include <cstddef>
#include "value_objects/pud_derivation_id.hpp"

struct pud_solution {
    size_t conjecture;
    pud_derivation_id derivation;
    auto operator<=>(const pud_solution&) const = default;
};

#endif
