#ifndef RESOLVER_HPP
#define RESOLVER_HPP

#include "value_objects/lineage.hpp"

template<typename IGoalDeactivator, typename IActivateSubgoalsAndCandidates,
         typename IDeactivateGoalCandidates, typename ISetChosenGoalCandidate>
struct resolver {
    resolver(IGoalDeactivator&, IActivateSubgoalsAndCandidates&,
             IDeactivateGoalCandidates&, ISetChosenGoalCandidate&);
    bool resolve(const resolution_lineage*);
private:
    IGoalDeactivator& goal_deactivator;
    IActivateSubgoalsAndCandidates& activate_subgoals_and_candidates;
    IDeactivateGoalCandidates& deactivate_goal_candidates;
    ISetChosenGoalCandidate& set_chosen_goal_candidate;
};

template<typename IGD, typename IASC, typename IDGC, typename ISGC>
resolver<IGD, IASC, IDGC, ISGC>::resolver(IGD& gd, IASC& asc, IDGC& dgc, ISGC& sgc)
    : goal_deactivator(gd), activate_subgoals_and_candidates(asc),
      deactivate_goal_candidates(dgc), set_chosen_goal_candidate(sgc) {}

template<typename IGD, typename IASC, typename IDGC, typename ISGC>
bool resolver<IGD, IASC, IDGC, ISGC>::resolve(const resolution_lineage* rl) {
    if (!activate_subgoals_and_candidates.activate_subgoals_and_candidates(rl))
        return false;
    const goal_lineage* gl = rl->parent;
    deactivate_goal_candidates.deactivate_goal_candidates(gl);
    goal_deactivator.deactivate(gl);
    set_chosen_goal_candidate.set(rl->parent, rl->idx);
    return true;
}

#endif
