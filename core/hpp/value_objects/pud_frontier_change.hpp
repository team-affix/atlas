#ifndef PUD_FRONTIER_CHANGE_HPP
#define PUD_FRONTIER_CHANGE_HPP

#include <compare>
#include <vector>
#include "value_objects/pud_derivation_id.hpp"
#include "value_objects/pud_unit_goal.hpp"

struct pud_frontier_change {
    std::vector<pud_derivation_id> opened;
    std::vector<pud_derivation_id> proven;
    std::vector<pud_derivation_id> refuted;
    std::vector<pud_unit_goal> made_unit;
    auto operator<=>(const pud_frontier_change&) const = default;
};

#endif
