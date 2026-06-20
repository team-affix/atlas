#ifndef GOAL_CANDIDATES_DEACTIVATOR_HPP
#define GOAL_CANDIDATES_DEACTIVATOR_HPP

#include "value_objects/lineage.hpp"

template<typename IGoalCandidateRules, typename ILineagePool, typename ICandidateDeactivator>
struct goal_candidates_deactivator {
    goal_candidates_deactivator(IGoalCandidateRules&, ILineagePool&, ICandidateDeactivator&);
    void deactivate_goal_candidates(const goal_lineage*);
private:
    IGoalCandidateRules& get_goal_candidate_rule_ids;
    ILineagePool& make_resolution_lineage;
    ICandidateDeactivator& candidate_deactivator;
};

template<typename IGCR, typename ILP, typename ICD>
goal_candidates_deactivator<IGCR, ILP, ICD>::goal_candidates_deactivator(
    IGCR& gcr, ILP& lp, ICD& cd)
    : get_goal_candidate_rule_ids(gcr), make_resolution_lineage(lp),
      candidate_deactivator(cd) {}

template<typename IGCR, typename ILP, typename ICD>
void goal_candidates_deactivator<IGCR, ILP, ICD>::deactivate_goal_candidates(
    const goal_lineage* gl) {
    auto candidate_rules = get_goal_candidate_rule_ids.get(gl).copy();
    auto cand_it = candidate_rules.iterate();
    while (!cand_it.done()) {
        cand_it.resume();
        if (!cand_it.has_yield()) continue;
        candidate_deactivator.deactivate(
            make_resolution_lineage.make_resolution_lineage(gl, cand_it.consume_yield()));
    }
}

#endif
