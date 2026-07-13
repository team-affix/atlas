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
    IGoalDeactivator& goal_deactivator_;
    IActivateSubgoalsAndCandidates& activate_subgoals_and_candidates_;
    IDeactivateGoalCandidates& deactivate_goal_candidates_;
    ISetChosenGoalCandidate& set_chosen_goal_candidate_;
};

template<typename IGD, typename IASC, typename IDGC, typename ISGC>
resolver<IGD, IASC, IDGC, ISGC>::resolver(IGD& gd, IASC& asc, IDGC& dgc, ISGC& sgc)
    : goal_deactivator_(gd), activate_subgoals_and_candidates_(asc), deactivate_goal_candidates_(dgc), set_chosen_goal_candidate_(sgc) {}

template<typename IGD, typename IASC, typename IDGC, typename ISGC>
bool resolver<IGD, IASC, IDGC, ISGC>::resolve(const resolution_lineage* rl) {
    if (!activate_subgoals_and_candidates_.activate_subgoals_and_candidates(rl))
        return false;
    const goal_lineage* gl = rl->parent;
    deactivate_goal_candidates_.deactivate_goal_candidates(gl);
    goal_deactivator_.deactivate(gl);
    set_chosen_goal_candidate_.set(rl->parent, rl->idx);
    return true;
}

#endif
