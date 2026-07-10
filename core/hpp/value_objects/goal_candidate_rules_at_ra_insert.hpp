#ifndef GOAL_CANDIDATE_RULES_AT_RA_INSERT_HPP
#define GOAL_CANDIDATE_RULES_AT_RA_INSERT_HPP

#include <compare>
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

struct goal_candidate_rules_at_ra_insert {
    const goal_lineage* gl;
    rule_id rule;
    auto operator<=>(const goal_candidate_rules_at_ra_insert&) const = default;
};

#endif
