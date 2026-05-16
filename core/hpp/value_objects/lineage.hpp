#ifndef LINEAGE_HPP
#define LINEAGE_HPP

#include <cstddef>
#include <compare>

struct resolution_lineage;

struct goal_lineage {
    const resolution_lineage* parent;
    size_t idx;
    auto operator<=>(const goal_lineage&) const = default;
};

struct resolution_lineage {
    const goal_lineage* parent;
    size_t idx;
    auto operator<=>(const resolution_lineage&) const = default;
};

#endif
