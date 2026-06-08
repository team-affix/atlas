#include "infrastructure/srt_subgoals_activator.hpp"

srt_subgoals_activator::srt_subgoals_activator(locator& loc)
    :
    link_srt_goal_batch_parent_(loc.locate<i_srt_link_goal_batch_parent>()),
    flush_srt_goal_batch_(loc.locate<i_srt_flush_goal_batch>()),
    subgoals_activator_(loc.locate<subgoals_activator>())
    {}

bool srt_subgoals_activator::activate_subgoals_and_candidates(const resolution_lineage* rl) {
    subgoals_activator_.activate_subgoals_and_candidates(rl);
    link_srt_goal_batch_parent_.link_srt_goal_batch_parent(rl->parent);
    flush_srt_goal_batch_.flush_srt_goal_batch();
    return true;
}
