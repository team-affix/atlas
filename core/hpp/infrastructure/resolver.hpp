#ifndef RESOLVER_HPP
#define RESOLVER_HPP

#include "../interfaces/i_resolver.hpp"
#include "../interfaces/i_make_goal_lineage.hpp"
#include "../interfaces/i_make_resolution_lineage.hpp"
#include "../interfaces/i_goal_activator.hpp"
#include "../interfaces/i_goal_deactivator.hpp"
#include "../interfaces/i_get_goal_db_rules.hpp"
#include "../interfaces/i_get_goal_candidate_rules.hpp"
#include "../interfaces/i_candidate_activator.hpp"
#include "../interfaces/i_candidate_deactivator.hpp"
#include "../interfaces/i_conflict_detector.hpp"
#include "../interfaces/i_detect_unit_goal.hpp"
#include "../interfaces/i_push_unit_goal.hpp"

struct resolver : i_resolver {
    resolver(
        i_make_goal_lineage& make_goal_lineage,
        i_make_resolution_lineage& make_resolution_lineage,
        i_goal_activator& goal_activator,
        i_goal_deactivator& goal_deactivator,
        i_get_goal_db_rules& get_goal_db_rules,
        i_get_goal_candidate_rules& get_goal_candidate_rules,
        i_candidate_activator& candidate_activator,
        i_candidate_deactivator& candidate_deactivator,
        i_conflict_detector& conflict_detector,
        i_detect_unit_goal& ugd,
        i_push_unit_goal& push_unit_goal);
    bool resolve(const resolution_lineage*) override;
private:
    i_make_goal_lineage& make_goal_lineage;
    i_make_resolution_lineage& make_resolution_lineage;
    i_goal_activator& goal_activator;
    i_goal_deactivator& goal_deactivator;
    i_get_goal_db_rules& get_goal_db_rules;
    i_get_goal_candidate_rules& get_goal_candidate_rules;
    i_candidate_activator& candidate_activator;
    i_candidate_deactivator& candidate_deactivator;
    i_conflict_detector& conflict_detector;
    i_detect_unit_goal& ugd;
    i_push_unit_goal& push_unit_goal;
};

#endif
