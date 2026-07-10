#ifndef GOAL_CANDIDATE_RULES_ACTION_HPP
#define GOAL_CANDIDATE_RULES_ACTION_HPP

#include <variant>
#include "value_objects/goal_candidate_rules_at_ra_erase.hpp"
#include "value_objects/goal_candidate_rules_at_ra_insert.hpp"
#include "value_objects/goal_candidate_rules_erase.hpp"
#include "value_objects/goal_candidate_rules_insert.hpp"

using goal_candidate_rules_action = std::variant<
    goal_candidate_rules_insert,
    goal_candidate_rules_erase,
    goal_candidate_rules_at_ra_insert,
    goal_candidate_rules_at_ra_erase>;

#endif
