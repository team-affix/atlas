#include "../../hpp/infrastructure/goal_deactivator.hpp"

goal_deactivator::goal_deactivator(i_unset_goal_expr& unset_goal_expr)
    : unset_goal_expr(unset_goal_expr) {}

void goal_deactivator::deactivate(const goal_lineage* gl) {
    unset_goal_expr.unset(gl);
}
