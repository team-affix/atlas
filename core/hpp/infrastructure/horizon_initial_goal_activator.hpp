#ifndef HORIZON_INITIAL_GOAL_ACTIVATOR_HPP
#define HORIZON_INITIAL_GOAL_ACTIVATOR_HPP

#include "infrastructure/locator.hpp"
#include "infrastructure/initial_goal_activator.hpp"
#include "interfaces/i_activate_initial_goal.hpp"
#include "interfaces/i_set_goal_weight.hpp"
#include "interfaces/i_get_initial_goal_weight.hpp"
#include "interfaces/i_make_initial_goal_lineage.hpp"

struct horizon_initial_goal_activator : i_activate_initial_goal {
    horizon_initial_goal_activator(locator& loc);
    void activate_initial_goal(subgoal_id idx) override;
private:
    initial_goal_activator& initial_goal_activator_;
    i_make_initial_goal_lineage& make_initial_goal_lineage_;
    i_set_goal_weight& set_goal_weight_;
    i_get_initial_goal_weight& get_initial_goal_weight_;
};

#endif
