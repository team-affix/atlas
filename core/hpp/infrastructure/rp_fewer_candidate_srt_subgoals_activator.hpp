#ifndef RP_FEWER_CANDIDATE_SRT_SUBGOALS_ACTIVATOR_HPP
#define RP_FEWER_CANDIDATE_SRT_SUBGOALS_ACTIVATOR_HPP

#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

template<typename IActivateSubgoalsAndCandidates, typename IGetRule,
         typename IMakeGoalLineage, typename IComputeActiveGoalValue,
         typename ISetActiveGoalValue>
struct rp_fewer_candidate_srt_subgoals_activator {
    rp_fewer_candidate_srt_subgoals_activator(
        IActivateSubgoalsAndCandidates&, IGetRule&, IMakeGoalLineage&,
        IComputeActiveGoalValue&, ISetActiveGoalValue&);
    bool activate_subgoals_and_candidates(const resolution_lineage*);
private:
    IActivateSubgoalsAndCandidates& activate_subgoals_and_candidates_;
    IGetRule& get_rule_;
    IMakeGoalLineage& make_goal_lineage_;
    IComputeActiveGoalValue& compute_active_goal_value_;
    ISetActiveGoalValue& set_active_goal_value_;
};

template<typename IASAC, typename IGR, typename IMGL, typename ICAV, typename ISAGV>
rp_fewer_candidate_srt_subgoals_activator<IASAC, IGR, IMGL, ICAV, ISAGV>::
rp_fewer_candidate_srt_subgoals_activator(
    IASAC& activate_subgoals_and_candidates, IGR& get_rule,
    IMGL& make_goal_lineage, ICAV& compute_active_goal_value,
    ISAGV& set_active_goal_value)
    : activate_subgoals_and_candidates_(activate_subgoals_and_candidates)
    , get_rule_(get_rule)
    , make_goal_lineage_(make_goal_lineage)
    , compute_active_goal_value_(compute_active_goal_value)
    , set_active_goal_value_(set_active_goal_value) {}

template<typename IASAC, typename IGR, typename IMGL, typename ICAV, typename ISAGV>
bool rp_fewer_candidate_srt_subgoals_activator<IASAC, IGR, IMGL, ICAV, ISAGV>::
activate_subgoals_and_candidates(const resolution_lineage* rl) {
    if (!activate_subgoals_and_candidates_.activate_subgoals_and_candidates(rl))
        return false;
    const rule* r = get_rule_.get_rule(rl->idx);
    for (subgoal_id body_idx = 0; body_idx < r->body.size(); ++body_idx) {
        const goal_lineage* gl = make_goal_lineage_.make_goal_lineage(rl, body_idx);
        set_active_goal_value_.set_active_goal_value(
            gl, compute_active_goal_value_.compute_active_goal_value(gl));
    }
    return true;
}

#endif
