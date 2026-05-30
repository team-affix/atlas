#ifndef INITIAL_GOAL_ACTIVATOR_HPP
#define INITIAL_GOAL_ACTIVATOR_HPP

#include "infrastructure/locator.hpp"
#include "interfaces/i_activate_initial_goal.hpp"
#include "interfaces/i_get_initial_goal_expr.hpp"
#include "interfaces/i_make_initial_goal_lineage.hpp"
#include "interfaces/i_set_goal_expr.hpp"
#include "interfaces/i_insert_active_goal.hpp"

struct initial_goal_activator : i_activate_initial_goal {
    initial_goal_activator(locator& loc);
    void activate_initial_goal(subgoal_id idx) override;
private:
    i_get_initial_goal_expr& get_initial_goal_expr;
    i_make_initial_goal_lineage& make_initial_goal_lineage;
    i_set_goal_expr& set_goal_expr;
    i_insert_active_goal& insert_active_goal;
};

#endif
