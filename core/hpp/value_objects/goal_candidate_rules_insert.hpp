#ifndef GOAL_CANDIDATE_RULES_INSERT_HPP
#define GOAL_CANDIDATE_RULES_INSERT_HPP

#include <compare>
#include "infrastructure/ra_rule_id_set.hpp"
#include "value_objects/lineage.hpp"

struct goal_candidate_rules_insert {
    const goal_lineage* gl;
    ra_rule_id_set value;
    auto operator<=>(const goal_candidate_rules_insert&) const = default;
};

#endif
