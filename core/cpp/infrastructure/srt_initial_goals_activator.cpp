#include "infrastructure/srt_initial_goals_activator.hpp"

srt_initial_goals_activator::srt_initial_goals_activator(locator& loc)
    :
    flush_srt_goal_batch_(loc.locate<i_srt_flush_goal_batch>()),
    initial_goals_activator_(loc.locate<initial_goals_activator>())
    {}

bool srt_initial_goals_activator::activate_initial_goals_and_candidates() {
    initial_goals_activator_.activate_initial_goals_and_candidates();
    flush_srt_goal_batch_.flush_srt_goal_batch();
    return true;
}
