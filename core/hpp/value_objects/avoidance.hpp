#ifndef AVOIDANCE_HPP
#define AVOIDANCE_HPP

#include <cstddef>
#include <vector>
#include "lineage.hpp"

struct avoidance {
    std::vector<const resolution_lineage*> members;
    size_t watcher_a_pos = 0;
    size_t watcher_b_pos = 1;
};

#endif
