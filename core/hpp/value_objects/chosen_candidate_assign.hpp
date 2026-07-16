#ifndef CHOSEN_CANDIDATE_ASSIGN_HPP
#define CHOSEN_CANDIDATE_ASSIGN_HPP

#include <compare>
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

struct chosen_candidate_assign {
    const goal_lineage* gl;
    rule_id value;
    auto operator<=>(const chosen_candidate_assign&) const = default;
};

#endif
