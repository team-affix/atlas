#ifndef PUD_FRONTIER_CHANGE_HPP
#define PUD_FRONTIER_CHANGE_HPP

#include <compare>
#include <vector>
#include "value_objects/pud_global_node_id.hpp"

struct pud_frontier_change {
    std::vector<pud_global_node_id> offspring;
    std::vector<pud_global_node_id> refuted;
    auto operator<=>(const pud_frontier_change&) const = default;
};

#endif
