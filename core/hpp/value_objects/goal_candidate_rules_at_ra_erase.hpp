#ifndef GOAL_CANDIDATE_RULES_AT_RA_ERASE_HPP
#define GOAL_CANDIDATE_RULES_AT_RA_ERASE_HPP

#include <compare>
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

struct goal_candidate_rules_at_ra_erase {
    const goal_lineage* gl;
    rule_id rule;
    auto operator<=>(const goal_candidate_rules_at_ra_erase&) const = default;
};

#endif
