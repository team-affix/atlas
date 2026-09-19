#ifndef PUD_FORCED_UNFOLD_HPP
#define PUD_FORCED_UNFOLD_HPP

#include <compare>
#include <cstddef>
#include <variant>
#include "value_objects/pud_rule_id.hpp"

struct pud_forced_unfold {
    struct unit {
        const pud_rule_id* leaf;
        size_t body_goal_idx;
        auto operator<=>(const unit&) const = default;
    };
    struct refuted {
        const pud_rule_id* leaf;
        auto operator<=>(const refuted&) const = default;
    };
    std::variant<unit, refuted> content;
    auto operator<=>(const pud_forced_unfold&) const = default;
};

#endif
