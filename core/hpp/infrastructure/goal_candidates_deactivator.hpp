#ifndef GOAL_CANDIDATES_DEACTIVATOR_HPP
#define GOAL_CANDIDATES_DEACTIVATOR_HPP

#include "value_objects/lineage.hpp"

template<typename IGetGoalCandidateRuleIds, typename IMakeResolutionLineage, typename ICandidateDeactivator>
struct goal_candidates_deactivator {
    goal_candidates_deactivator(IGetGoalCandidateRuleIds&, IMakeResolutionLineage&, ICandidateDeactivator&);
    void deactivate_goal_candidates(const goal_lineage*);
private:
    IGetGoalCandidateRuleIds& get_goal_candidate_rule_ids_;
    IMakeResolutionLineage& make_resolution_lineage_;
    ICandidateDeactivator& candidate_deactivator_;
};

template<typename IGCRI, typename IMRL, typename ICD>
goal_candidates_deactivator<IGCRI, IMRL, ICD>::goal_candidates_deactivator(
    IGCRI& gcr, IMRL& lp, ICD& cd)
    : get_goal_candidate_rule_ids_(gcr), make_resolution_lineage_(lp),
      candidate_deactivator_(cd) {}

template<typename IGCRI, typename IMRL, typename ICD>
void goal_candidates_deactivator<IGCRI, IMRL, ICD>::deactivate_goal_candidates(
    const goal_lineage* gl) {
    auto candidate_rules = get_goal_candidate_rule_ids_.get(gl).copy();
    auto cand_it = candidate_rules.iterate();
    while (!cand_it.done()) {
        cand_it.resume();
        if (!cand_it.has_yield()) continue;
        candidate_deactivator_.deactivate(
            make_resolution_lineage_.make_resolution_lineage(gl, cand_it.consume_yield()));
    }
}

#endif
