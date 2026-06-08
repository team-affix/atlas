#ifndef INITIAL_GOALS_ACTIVATOR_HPP
#define INITIAL_GOALS_ACTIVATOR_HPP

#include "infrastructure/locator.hpp"
#include "interfaces/i_activate_initial_goals_and_candidates.hpp"
#include "interfaces/i_get_initial_goal_count.hpp"
#include "interfaces/i_activate_initial_goal.hpp"
#include "interfaces/i_make_initial_goal_lineage.hpp"
#include "interfaces/i_activate_goal_candidates.hpp"

struct initial_goals_activator : i_activate_initial_goals_and_candidates {
    initial_goals_activator(locator& loc);
    bool activate_initial_goals_and_candidates() override;
private:
    i_get_initial_goal_count& get_initial_goal_count;
    i_activate_initial_goal& activate_initial_goal;
    i_make_initial_goal_lineage& make_initial_goal_lineage;
    i_activate_goal_candidates& activate_goal_candidates;
};

#endif
