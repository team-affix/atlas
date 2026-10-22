#ifndef PUD_SUBSTITUTION_HPP
#define PUD_SUBSTITUTION_HPP

#include <compare>
#include <cstdint>
#include "value_objects/expr.hpp"

// var_idx is a representative just before the substitution; value is written in the
// representatives of the environment the substitution produces.
struct pud_substitution {
    uint32_t var_idx;
    const expr* value;
    auto operator<=>(const pud_substitution&) const = default;
};

#endif
