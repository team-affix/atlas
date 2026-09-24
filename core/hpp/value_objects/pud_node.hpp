#ifndef PUD_NODE_HPP
#define PUD_NODE_HPP

#include <compare>
#include <vector>
#include <unordered_map>
#include <optional>
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_added_unification.hpp"

struct pud_node {
    bool refuted;
    om_interval interval;
    uint32_t lineage_variable_count;
    std::vector<pud_added_unification> added_unifications;
    std::vector<const expr*> added_body_goals;
    std::optional<std::unordered_map<const pud_rule_id*, pud_node>> children;
    auto operator<=>(const pud_node&) const = default;
};

#endif