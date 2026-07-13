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
    IGetGoalDbRuleIds& get_goal_db_rule_ids_;
    IMakeResolutionLineage& make_resolution_lineage_;
    ICandidateActivator& candidate_activator_;
    IConflictDetector& conflict_detector_;
    IUnitGoalDetector& ugd_;
    IPushUnitGoal& push_unit_goal_;
};

template<typename IGGDRI, typename IMRL, typename ICA, typename ICD, typename IUGD, typename IPUG>
goal_candidates_activator<IGGDRI, IMRL, ICA, ICD, IUGD, IPUG>::goal_candidates_activator(
    IGGDRI& db, IMRL& lp, ICA& ca, ICD& cd, IUGD& ugd, IPUG& ug)
    : get_goal_db_rule_ids_(db), make_resolution_lineage_(lp), candidate_activator_(ca), conflict_detector_(cd), ugd_(ugd), push_unit_goal_(ug) {}

template<typename IGGDRI, typename IMRL, typename ICA, typename ICD, typename IUGD, typename IPUG>
bool goal_candidates_activator<IGGDRI, IMRL, ICA, ICD, IUGD, IPUG>::activate_goal_candidates(
    const goal_lineage* gl) {
    auto& db_rules = get_goal_db_rule_ids_.get_candidate_rules(gl);
    auto db_it = db_rules.iterate();
    while (!db_it.done()) {
        db_it.resume();
        if (!db_it.has_yield()) continue;
        candidate_activator_.activate(
            make_resolution_lineage_.make_resolution_lineage(gl, db_it.consume_yield()));
    }
    if (conflict_detector_.detect(gl)) return false;
    if (ugd_.detect(gl)) push_unit_goal_.push(gl);
    return true;
}

#endif
