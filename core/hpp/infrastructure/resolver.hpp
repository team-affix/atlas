#ifndef RESOLVER_HPP
#define RESOLVER_HPP

#include "infrastructure/locator.hpp"
#include "interfaces/i_resolver.hpp"
#include "interfaces/i_make_goal_lineage.hpp"
#include "interfaces/i_make_resolution_lineage.hpp"
#include "interfaces/i_goal_activator.hpp"
#include "interfaces/i_goal_deactivator.hpp"
#include "interfaces/i_get_rule.hpp"
#include "interfaces/i_get_goal_db_rule_ids.hpp"
#include "interfaces/i_get_goal_candidate_rule_ids.hpp"
#include "interfaces/i_candidate_activator.hpp"
#include "interfaces/i_candidate_deactivator.hpp"
#include "interfaces/i_conflict_detector.hpp"
#include "interfaces/i_detect_unit_goal.hpp"
#include "interfaces/i_push_unit_goal.hpp"

struct resolver : i_resolver {
    resolver(locator& loc);
    bool resolve(const resolution_lineage*) override;
private:
    i_make_goal_lineage& make_goal_lineage;
    i_make_resolution_lineage& make_resolution_lineage;
    i_goal_activator& goal_activator;
    i_goal_deactivator& goal_deactivator;
    i_get_rule& get_rule;
    i_get_goal_db_rule_ids& get_goal_db_rule_ids;
    i_get_goal_candidate_rule_ids& get_goal_candidate_rule_ids;
    i_candidate_activator& candidate_activator;
    i_candidate_deactivator& candidate_deactivator;
    i_conflict_detector& conflict_detector;
    i_detect_unit_goal& ugd;
    i_push_unit_goal& push_unit_goal;
};

#endif
