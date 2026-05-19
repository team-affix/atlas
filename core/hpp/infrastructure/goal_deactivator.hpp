#ifndef GOAL_DEACTIVATOR_HPP
#define GOAL_DEACTIVATOR_HPP

#include "../interfaces/i_goal_deactivator.hpp"
#include "../interfaces/i_deactivate_goal_expr.hpp"

struct goal_deactivator : i_goal_deactivator {
    goal_deactivator(i_deactivate_goal_expr& dge);
    void deactivate(const goal_lineage*) override;
private:
    i_deactivate_goal_expr& dge;
};

#endif
