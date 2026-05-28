#include "infrastructure/goal_deactivator.hpp"

goal_deactivator::goal_deactivator(
    i_unset_goal_expr& unset_goal_expr,
    i_erase_goal_candidates& erase_goal_candidates)
    :
    unset_goal_expr(unset_goal_expr),
    erase_goal_candidates(erase_goal_candidates) {}

void goal_deactivator::deactivate(const goal_lineage* gl) {
    erase_goal_candidates.erase(gl);
    unset_goal_expr.unset(gl);
}
