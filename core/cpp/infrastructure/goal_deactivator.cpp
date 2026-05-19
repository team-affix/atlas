#include "../../hpp/infrastructure/goal_deactivator.hpp"

goal_deactivator::goal_deactivator(i_deactivate_goal_expr& dge)
    : dge(dge) {}

void goal_deactivator::deactivate(const goal_lineage*) {}
