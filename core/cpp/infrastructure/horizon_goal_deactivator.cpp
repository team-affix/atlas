#include "infrastructure/horizon_goal_deactivator.hpp"

horizon_goal_deactivator::horizon_goal_deactivator(locator& loc)
    : srt_goal_deactivator_(loc.locate<srt_goal_deactivator>()),
      erase_goal_weight_(loc.locate<i_erase_goal_weight>()) {}

void horizon_goal_deactivator::deactivate(const goal_lineage* gl) {
    erase_goal_weight_.erase(gl);
    srt_goal_deactivator_.deactivate(gl);
}
