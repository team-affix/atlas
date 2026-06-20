#ifndef RESOLVER_HPP
#define RESOLVER_HPP

#include "value_objects/lineage.hpp"

template<typename IGoalDeactivator, typename IActivateSubgoalsAndCandidates, typename IDeactivateGoalCandidates>
struct resolver {
    resolver(IGoalDeactivator&, IActivateSubgoalsAndCandidates&, IDeactivateGoalCandidates&);
    bool resolve(const resolution_lineage*);
private:
    IGoalDeactivator& goal_deactivator;
    IActivateSubgoalsAndCandidates& activate_subgoals_and_candidates;
    IDeactivateGoalCandidates& deactivate_goal_candidates;
};

template<typename IGD, typename IASC, typename IDGC>
resolver<IGD, IASC, IDGC>::resolver(IGD& gd, IASC& asc, IDGC& dgc)
    : goal_deactivator(gd), activate_subgoals_and_candidates(asc),
      deactivate_goal_candidates(dgc) {}

template<typename IGD, typename IASC, typename IDGC>
bool resolver<IGD, IASC, IDGC>::resolve(const resolution_lineage* rl) {
    if (!activate_subgoals_and_candidates.activate_subgoals_and_candidates(rl))
        return false;
    const goal_lineage* gl = rl->parent;
    deactivate_goal_candidates.deactivate_goal_candidates(gl);
    goal_deactivator.deactivate(gl);
    return true;
}

#endif
