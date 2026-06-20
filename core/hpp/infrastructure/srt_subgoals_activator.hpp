#ifndef SRT_SUBGOALS_ACTIVATOR_HPP
#define SRT_SUBGOALS_ACTIVATOR_HPP

#include "value_objects/lineage.hpp"

template<typename IFlushSrtGoalBatch, typename ISubgoalsActivator>
struct srt_subgoals_activator {
    srt_subgoals_activator(IFlushSrtGoalBatch&, ISubgoalsActivator&);
    bool activate_subgoals_and_candidates(const resolution_lineage*);
private:
    IFlushSrtGoalBatch& srt_active_goals_;
    ISubgoalsActivator& subgoals_activator_;
};

template<typename IFSGB, typename ISA>
srt_subgoals_activator<IFSGB, ISA>::srt_subgoals_activator(IFSGB& sag, ISA& sa)
    : srt_active_goals_(sag), subgoals_activator_(sa) {}

template<typename IFSGB, typename ISA>
bool srt_subgoals_activator<IFSGB, ISA>::activate_subgoals_and_candidates(
    const resolution_lineage* rl) {
    if (!subgoals_activator_.activate_subgoals_and_candidates(rl)) return false;
    srt_active_goals_.link_srt_goal_batch_parent(rl->parent);
    srt_active_goals_.flush_srt_goal_batch();
    return true;
}

#endif
