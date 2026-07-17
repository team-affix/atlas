#ifndef FEWER_CANDIDATE_GOAL_CANDIDATES_ACTIVATOR_HPP
#define FEWER_CANDIDATE_GOAL_CANDIDATES_ACTIVATOR_HPP

#include "value_objects/lineage.hpp"

template<typename IActivateGoalCandidates,
         typename IComputeActiveGoalValue,
         typename ISetActiveGoalValue>
struct fewer_candidate_goal_candidates_activator {
    fewer_candidate_goal_candidates_activator(
        IActivateGoalCandidates&, IComputeActiveGoalValue&, ISetActiveGoalValue&);
    bool activate_goal_candidates(const goal_lineage* gl);
private:
    IActivateGoalCandidates& activate_goal_candidates_;
    IComputeActiveGoalValue& compute_active_goal_value_;
    ISetActiveGoalValue& set_active_goal_value_;
};

template<typename IAGC, typename ICAV, typename ISAGV>
fewer_candidate_goal_candidates_activator<IAGC, ICAV, ISAGV>::
fewer_candidate_goal_candidates_activator(
    IAGC& activate_goal_candidates, ICAV& compute_active_goal_value,
    ISAGV& set_active_goal_value)
    : activate_goal_candidates_(activate_goal_candidates)
    , compute_active_goal_value_(compute_active_goal_value)
    , set_active_goal_value_(set_active_goal_value) {}

template<typename IAGC, typename ICAV, typename ISAGV>
bool fewer_candidate_goal_candidates_activator<IAGC, ICAV, ISAGV>::
activate_goal_candidates(const goal_lineage* gl) {
    if (!activate_goal_candidates_.activate_goal_candidates(gl)) return false;
    set_active_goal_value_.set_active_goal_value(
        gl, compute_active_goal_value_.compute_active_goal_value(gl));
    return true;
}

#endif
