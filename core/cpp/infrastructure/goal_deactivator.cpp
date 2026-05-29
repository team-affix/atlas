#include "infrastructure/goal_deactivator.hpp"

goal_deactivator::goal_deactivator(locator& loc)
    :
    unset_goal_expr(loc.locate<i_unset_goal_expr>()),
    erase_goal_candidates(loc.locate<i_erase_goal_candidates>()),
    erase_active_goal(loc.locate<i_erase_active_goal>()) {}

void goal_deactivator::deactivate(const goal_lineage* gl) {
    erase_goal_candidates.erase(gl);
    unset_goal_expr.unset(gl);
    erase_active_goal.erase_active_goal(gl);
}
