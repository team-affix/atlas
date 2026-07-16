#ifndef GOAL_CANDIDATE_RULES_ERASE_HPP
#define GOAL_CANDIDATE_RULES_ERASE_HPP

#include <compare>
#include "infrastructure/ra_rule_id_set.hpp"
#include "value_objects/lineage.hpp"

struct goal_candidate_rules_erase {
    const goal_lineage* gl;
    ra_rule_id_set value;
    auto operator<=>(const goal_candidate_rules_erase&) const = default;
};

#endif
