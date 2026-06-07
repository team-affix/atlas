#ifndef GOAL_CANDIDATES_ACTIVATOR_HPP
#define GOAL_CANDIDATES_ACTIVATOR_HPP

#include "infrastructure/locator.hpp"
#include "interfaces/i_activate_goal_candidates.hpp"
#include "interfaces/i_get_goal_db_rule_ids.hpp"
#include "interfaces/i_make_resolution_lineage.hpp"
#include "interfaces/i_candidate_activator.hpp"
#include "interfaces/i_conflict_detector.hpp"
#include "interfaces/i_detect_unit_goal.hpp"
#include "interfaces/i_push_unit_goal.hpp"

struct goal_candidates_activator : i_activate_goal_candidates {
    goal_candidates_activator(locator& loc);
    bool activate_goal_candidates(const goal_lineage*) override;
private:
    i_get_goal_db_rule_ids& get_goal_db_rule_ids;
    i_make_resolution_lineage& make_resolution_lineage;
    i_candidate_activator& candidate_activator;
    i_conflict_detector& conflict_detector;
    i_detect_unit_goal& ugd;
    i_push_unit_goal& push_unit_goal;
};

#endif
