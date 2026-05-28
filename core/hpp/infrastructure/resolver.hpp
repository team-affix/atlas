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
#include "../interfaces/i_rule_set.hpp"
#include "../interfaces/i_conflict_detector.hpp"
#include "../interfaces/i_unit_goal_detector.hpp"
#include "../interfaces/i_push_unit_goal.hpp"

struct resolver : i_resolver {
    resolver(
        i_make_goal_lineage& make_goal_lineage,
        i_make_resolution_lineage& make_resolution_lineage,
        i_goal_activator& goal_activator,
        i_goal_deactivator& goal_deactivator,
        i_get_goal_db_rules& ggdr,
        i_get_goal_candidate_rules& ggcr,
        i_candidate_activator& ca,
        i_candidate_deactivator& cd,
        i_conflict_detector& conflict_detector,
        i_unit_goal_detector& ugd,
        i_push_unit_goal& push_unit_goal);
    bool resolve(const resolution_lineage*) override;
private:
    void activate_candidates(const goal_lineage*, i_rule_set&);
    void deactivate_candidates(const goal_lineage*, i_rule_set&);
    i_make_goal_lineage& make_goal_lineage;
    i_make_resolution_lineage& make_resolution_lineage;
    i_goal_activator& goal_activator;
    i_goal_deactivator& goal_deactivator;
    i_get_goal_db_rules& ggdr;
    i_get_goal_candidate_rules& ggcr;
    i_candidate_activator& ca;
    i_candidate_deactivator& cd;
    i_conflict_detector& conflict_detector;
    i_unit_goal_detector& ugd;
    i_push_unit_goal& push_unit_goal;
};

#endif
