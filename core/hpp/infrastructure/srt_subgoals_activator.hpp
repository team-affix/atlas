#ifndef SRT_SUBGOALS_ACTIVATOR_HPP
#define SRT_SUBGOALS_ACTIVATOR_HPP

#include "value_objects/lineage.hpp"

template<typename ILinkSrtGoalBatchParent, typename IFlushSrtGoalBatch, typename ISubgoalsActivator>
struct srt_subgoals_activator {
    srt_subgoals_activator(ILinkSrtGoalBatchParent&, IFlushSrtGoalBatch&, ISubgoalsActivator&);
    bool activate_subgoals_and_candidates(const resolution_lineage*);
private:
    ILinkSrtGoalBatchParent& link_srt_goal_batch_parent_;
    IFlushSrtGoalBatch& flush_srt_goal_batch_;
    ISubgoalsActivator& subgoals_activator_;
};

template<typename ILSGBP, typename IFSGB, typename ISA>
srt_subgoals_activator<ILSGBP, IFSGB, ISA>::srt_subgoals_activator(
    ILSGBP& lsgbp, IFSGB& fsgb, ISA& sa)
    : link_srt_goal_batch_parent_(lsgbp), flush_srt_goal_batch_(fsgb), subgoals_activator_(sa) {}

template<typename ILSGBP, typename IFSGB, typename ISA>
bool srt_subgoals_activator<ILSGBP, IFSGB, ISA>::activate_subgoals_and_candidates(
    const resolution_lineage* rl) {
    if (!subgoals_activator_.activate_subgoals_and_candidates(rl)) return false;
    link_srt_goal_batch_parent_.link_srt_goal_batch_parent(rl->parent);
    flush_srt_goal_batch_.flush_srt_goal_batch();
    return true;
}

#endif
