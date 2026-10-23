#ifndef PUD_FRONTIER_CHANGE_HPP
#define PUD_FRONTIER_CHANGE_HPP

#include <compare>
#include <vector>
#include "value_objects/pud_global_node_id.hpp"

// What one change to the frontier did: the leaves it put on the frontier that are still alive, and
// every leaf refuted anywhere as a result, through query cascades included.
struct pud_frontier_change {
    std::vector<pud_global_node_id> offspring;
    std::vector<pud_global_node_id> refuted;
    auto operator<=>(const pud_frontier_change&) const = default;
};

#endif
