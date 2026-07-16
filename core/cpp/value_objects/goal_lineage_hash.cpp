#include "value_objects/goal_lineage_hash.hpp"

#include <functional>

size_t goal_lineage_hash::operator()(const goal_lineage& lineage) const noexcept {
    size_t seed = std::hash<const resolution_lineage*>{}(lineage.parent);
    seed ^= std::hash<subgoal_id>{}(lineage.idx) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
}
