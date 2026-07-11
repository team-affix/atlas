#ifndef GOAL_LINEAGE_HASH_HPP
#define GOAL_LINEAGE_HASH_HPP

#include <cstddef>
#include <functional>
#include "lineage.hpp"

struct goal_lineage_hash {
    size_t operator()(const goal_lineage& lineage) const noexcept {
        size_t seed = std::hash<const resolution_lineage*>{}(lineage.parent);
        seed ^= std::hash<subgoal_id>{}(lineage.idx) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

#endif
