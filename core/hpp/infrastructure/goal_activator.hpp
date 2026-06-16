#ifndef GOAL_ACTIVATOR_HPP
#define GOAL_ACTIVATOR_HPP

#include "infrastructure/locator.hpp"
#include "interfaces/i_goal_activator.hpp"
#include "interfaces/i_set_goal_expr.hpp"
#include "interfaces/i_insert_goal_candidates.hpp"
#include "interfaces/i_insert_active_goal.hpp"
#include "interfaces/i_get_candidate_frame_offset.hpp"
#include "interfaces/i_get_resolution_rule.hpp"

struct goal_activator : i_goal_activator {
    goal_activator(locator& loc);
    void activate(const goal_lineage*) override;
private:
    i_set_goal_expr& set_goal_expr;
    i_insert_goal_candidates& insert_goal_candidates;
    i_insert_active_goal& insert_active_goal;
    i_get_candidate_frame_offset& get_candidate_frame_offset;
    i_get_resolution_rule& get_resolution_rule;
};

#endif
