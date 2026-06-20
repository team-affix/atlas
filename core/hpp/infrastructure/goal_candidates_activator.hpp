#ifndef GOAL_CANDIDATES_ACTIVATOR_HPP
#define GOAL_CANDIDATES_ACTIVATOR_HPP

#include "value_objects/lineage.hpp"

template<typename IDb, typename ILineagePool, typename ICandidateActivator,
         typename IConflictDetector, typename IUnitGoalDetector, typename IUnitGoals>
struct goal_candidates_activator {
    goal_candidates_activator(IDb&, ILineagePool&, ICandidateActivator&,
                              IConflictDetector&, IUnitGoalDetector&, IUnitGoals&);
    bool activate_goal_candidates(const goal_lineage*);
private:
    IDb& get_goal_db_rule_ids;
    ILineagePool& make_resolution_lineage;
    ICandidateActivator& candidate_activator;
    IConflictDetector& conflict_detector;
    IUnitGoalDetector& ugd;
    IUnitGoals& push_unit_goal;
};

template<typename IDB, typename ILP, typename ICA, typename ICD, typename IUGD, typename IUG>
goal_candidates_activator<IDB, ILP, ICA, ICD, IUGD, IUG>::goal_candidates_activator(
    IDB& db, ILP& lp, ICA& ca, ICD& cd, IUGD& ugd, IUG& ug)
    : get_goal_db_rule_ids(db), make_resolution_lineage(lp), candidate_activator(ca),
      conflict_detector(cd), ugd(ugd), push_unit_goal(ug) {}

template<typename IDB, typename ILP, typename ICA, typename ICD, typename IUGD, typename IUG>
bool goal_candidates_activator<IDB, ILP, ICA, ICD, IUGD, IUG>::activate_goal_candidates(
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
