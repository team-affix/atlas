#ifndef CHOSEN_CANDIDATE_INSERT_HPP
#define CHOSEN_CANDIDATE_INSERT_HPP

#include <compare>
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

struct chosen_candidate_insert {
    const goal_lineage* gl;
    rule_id rule;
    auto operator<=>(const chosen_candidate_insert&) const = default;
};

#endif
