#ifndef PUD_CHILD_DELTA_HPP
#define PUD_CHILD_DELTA_HPP

#include <compare>
#include <cstdint>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/pud_substitution.hpp"

// What a child adds on top of its parent: its substitutions, its body goals and how many
// fresh variables they introduce.
struct pud_child_delta {
    std::vector<pud_substitution> substitutions;
    std::vector<const expr*> body_goals;
    uint32_t var_count;
    auto operator<=>(const pud_child_delta&) const = default;
};

#endif
