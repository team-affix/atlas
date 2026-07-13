#include "value_objects/resolution_lineage_hash.hpp"

#include <functional>

size_t resolution_lineage_hash::operator()(const resolution_lineage& lineage) const noexcept {
    size_t seed = std::hash<const goal_lineage*>{}(lineage.parent);
    seed ^= std::hash<rule_id>{}(lineage.idx) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
}
