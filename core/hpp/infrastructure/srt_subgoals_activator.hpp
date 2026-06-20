#ifndef SRT_SUBGOALS_ACTIVATOR_HPP
#define SRT_SUBGOALS_ACTIVATOR_HPP

#include "value_objects/lineage.hpp"

template<typename ISrtActiveGoals, typename ISubgoalsActivator>
struct srt_subgoals_activator {
    srt_subgoals_activator(ISrtActiveGoals&, ISubgoalsActivator&);
    bool activate_subgoals_and_candidates(const resolution_lineage*);
private:
    ISrtActiveGoals& srt_active_goals_;
    ISubgoalsActivator& subgoals_activator_;
};

template<typename ISAG, typename ISA>
srt_subgoals_activator<ISAG, ISA>::srt_subgoals_activator(ISAG& sag, ISA& sa)
    : srt_active_goals_(sag), subgoals_activator_(sa) {}

template<typename ISAG, typename ISA>
bool srt_subgoals_activator<ISAG, ISA>::activate_subgoals_and_candidates(
    const resolution_lineage* rl) {
    if (!subgoals_activator_.activate_subgoals_and_candidates(rl)) return false;
    srt_active_goals_.link_srt_goal_batch_parent(rl->parent);
    srt_active_goals_.flush_srt_goal_batch();
    return true;
}

#endif
