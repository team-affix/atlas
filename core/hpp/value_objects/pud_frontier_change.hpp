#ifndef PUD_FRONTIER_CHANGE_HPP
#define PUD_FRONTIER_CHANGE_HPP

#include <compare>
#include <vector>
#include "value_objects/pud_node_id.hpp"

// What one operation did to the frontier: the leaves it produced that still have open goals, those it
// produced with none left, and every leaf it refuted anywhere.
struct pud_frontier_change {
    std::vector<pud_node_id> opened;
    std::vector<pud_node_id> proven;
    std::vector<pud_node_id> refuted;
    auto operator<=>(const pud_frontier_change&) const = default;
};

#endif
