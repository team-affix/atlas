#ifndef GOAL_CANDIDATES_ACTIVATOR_HPP
#define GOAL_CANDIDATES_ACTIVATOR_HPP

#include "value_objects/lineage.hpp"

template<typename IGetGoalDbRuleIds, typename IMakeResolutionLineage, typename ICandidateActivator,
         typename IConflictDetector, typename IUnitGoalDetector, typename IPushUnitGoal>
struct goal_candidates_activator {
    goal_candidates_activator(IGetGoalDbRuleIds&, IMakeResolutionLineage&, ICandidateActivator&,
                              IConflictDetector&, IUnitGoalDetector&, IPushUnitGoal&);
    bool activate_goal_candidates(const goal_lineage*);
private:
    IGetGoalDbRuleIds& get_goal_db_rule_ids;
    IMakeResolutionLineage& make_resolution_lineage;
    ICandidateActivator& candidate_activator;
    IConflictDetector& conflict_detector;
    IUnitGoalDetector& ugd;
    IPushUnitGoal& push_unit_goal;
};

template<typename IGGDRI, typename IMRL, typename ICA, typename ICD, typename IUGD, typename IPUG>
goal_candidates_activator<IGGDRI, IMRL, ICA, ICD, IUGD, IPUG>::goal_candidates_activator(
    IGGDRI& db, IMRL& lp, ICA& ca, ICD& cd, IUGD& ugd, IPUG& ug)
    : get_goal_db_rule_ids(db), make_resolution_lineage(lp), candidate_activator(ca),
      conflict_detector(cd), ugd(ugd), push_unit_goal(ug) {}

template<typename IGGDRI, typename IMRL, typename ICA, typename ICD, typename IUGD, typename IPUG>
bool goal_candidates_activator<IGGDRI, IMRL, ICA, ICD, IUGD, IPUG>::activate_goal_candidates(
    const goal_lineage* gl) {
    auto& db_rules = get_goal_db_rule_ids.get(gl);
    auto db_it = db_rules.iterate();
    while (!db_it.done()) {
        db_it.resume();
        if (!db_it.has_yield()) continue;
        candidate_activator.activate(
            make_resolution_lineage.make_resolution_lineage(gl, db_it.consume_yield()));
    }
    if (conflict_detector.detect(gl)) return false;
    if (ugd.detect(gl)) push_unit_goal.push(gl);
    return true;
}

#endif
