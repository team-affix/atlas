#ifndef FEWER_CANDIDATE_SRT_SUBGOALS_ACTIVATOR_HPP
#define FEWER_CANDIDATE_SRT_SUBGOALS_ACTIVATOR_HPP

#include "value_objects/lineage.hpp"

template<typename ISubgoalsActivator, typename ILinkSrtGoalBatchParent,
         typename IIterateSrtGoalBatch, typename IFlushSrtGoalBatch,
         typename IComputeActiveGoalValue, typename ISetActiveGoalValue>
struct fewer_candidate_srt_subgoals_activator {
    fewer_candidate_srt_subgoals_activator(
        ISubgoalsActivator&, ILinkSrtGoalBatchParent&, IIterateSrtGoalBatch&,
        IFlushSrtGoalBatch&, IComputeActiveGoalValue&, ISetActiveGoalValue&);
    bool activate_subgoals_and_candidates(const resolution_lineage*);
private:
    ISubgoalsActivator& subgoals_activator_;
    ILinkSrtGoalBatchParent& link_srt_goal_batch_parent_;
    IIterateSrtGoalBatch& iterate_srt_goal_batch_;
    IFlushSrtGoalBatch& flush_srt_goal_batch_;
    IComputeActiveGoalValue& compute_active_goal_value_;
    ISetActiveGoalValue& set_active_goal_value_;
};

template<typename ISA, typename ILSP, typename IISB, typename IFSGB,
         typename ICAV, typename ISAGV>
fewer_candidate_srt_subgoals_activator<ISA, ILSP, IISB, IFSGB, ICAV, ISAGV>::
fewer_candidate_srt_subgoals_activator(
    ISA& subgoals_activator, ILSP& link_srt_goal_batch_parent,
    IISB& iterate_srt_goal_batch, IFSGB& flush_srt_goal_batch,
    ICAV& compute_active_goal_value, ISAGV& set_active_goal_value)
    : subgoals_activator_(subgoals_activator)
    , link_srt_goal_batch_parent_(link_srt_goal_batch_parent)
    , iterate_srt_goal_batch_(iterate_srt_goal_batch)
    , flush_srt_goal_batch_(flush_srt_goal_batch)
    , compute_active_goal_value_(compute_active_goal_value)
    , set_active_goal_value_(set_active_goal_value) {}

template<typename ISA, typename ILSP, typename IISB, typename IFSGB,
         typename ICAV, typename ISAGV>
bool fewer_candidate_srt_subgoals_activator<ISA, ILSP, IISB, IFSGB, ICAV, ISAGV>::
activate_subgoals_and_candidates(const resolution_lineage* rl) {
    if (!subgoals_activator_.activate_subgoals_and_candidates(rl)) return false;
    link_srt_goal_batch_parent_.link_srt_goal_batch_parent(rl->parent);
    auto sm = iterate_srt_goal_batch_.iterate_srt_goal_batch();
    while (!sm.done()) {
        sm.resume();
        if (sm.has_yield()) {
            const goal_lineage* gl = sm.consume_yield();
            set_active_goal_value_.set_active_goal_value(
                gl, compute_active_goal_value_.compute_active_goal_value(gl));
        }
    }
    flush_srt_goal_batch_.flush_srt_goal_batch();
    return true;
}

#endif
