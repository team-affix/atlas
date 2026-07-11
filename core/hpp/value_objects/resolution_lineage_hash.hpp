#ifndef RESOLUTION_LINEAGE_HASH_HPP
#define RESOLUTION_LINEAGE_HASH_HPP

#include <cstddef>
#include <functional>
#include "lineage.hpp"

struct resolution_lineage_hash {
    size_t operator()(const resolution_lineage& lineage) const noexcept {
        size_t seed = std::hash<const goal_lineage*>{}(lineage.parent);
        seed ^= std::hash<rule_id>{}(lineage.idx) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

#endif
