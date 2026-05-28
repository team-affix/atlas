#ifndef LINEAGE_HPP
#define LINEAGE_HPP

#include <compare>
#include <cstddef>
#include "expr.hpp"
#include "rule.hpp"

struct resolution_lineage;

using subgoal_id = size_t;
using rule_id = size_t;

struct goal_lineage {
    const resolution_lineage* parent;
    subgoal_id idx;
    auto operator<=>(const goal_lineage&) const = default;
};

struct resolution_lineage {
    const goal_lineage* parent;
    rule_id idx;
    auto operator<=>(const resolution_lineage&) const = default;
};

#endif
