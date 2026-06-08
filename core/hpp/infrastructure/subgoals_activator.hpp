#ifndef SUBGOALS_ACTIVATOR_HPP
#define SUBGOALS_ACTIVATOR_HPP

#include "infrastructure/locator.hpp"
#include "interfaces/i_activate_subgoals_and_candidates.hpp"
#include "interfaces/i_make_goal_lineage.hpp"
#include "interfaces/i_goal_activator.hpp"
#include "interfaces/i_get_rule.hpp"
#include "interfaces/i_activate_goal_candidates.hpp"

struct subgoals_activator : i_activate_subgoals_and_candidates {
    subgoals_activator(locator& loc);
    bool activate_subgoals_and_candidates(const resolution_lineage*) override;
private:
    i_make_goal_lineage& make_goal_lineage;
    i_goal_activator& goal_activator;
    i_get_rule& get_rule;
    i_activate_goal_candidates& activate_goal_candidates;
};

#endif
