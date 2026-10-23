#ifndef PUD_FRONTIER_CHANGE_HPP
#define PUD_FRONTIER_CHANGE_HPP

#include <compare>
#include <vector>
#include "value_objects/pud_node_id.hpp"

// What one expand or root-open did to the tree: new child node ids from that step, and every leaf
// refuted anywhere as a result (including query cascades).
struct pud_frontier_change {
    std::vector<pud_node_id> offspring;
    std::vector<pud_node_id> refuted;
    auto operator<=>(const pud_frontier_change&) const = default;
};

#endif
