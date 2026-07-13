#ifndef AVOIDANCE_HPP
#define AVOIDANCE_HPP

#include <compare>
#include <cstddef>
#include <vector>
#include "lineage.hpp"

struct avoidance {
    avoidance();
    avoidance(std::vector<const resolution_lineage*> members, size_t watcher_a_pos, size_t watcher_b_pos);
    std::vector<const resolution_lineage*> members;
    size_t watcher_a_pos;
    size_t watcher_b_pos;
    auto operator<=>(const avoidance&) const = default;
};

#endif
