#ifndef GOAL_DEACTIVATOR_HPP
#define GOAL_DEACTIVATOR_HPP

#include "../interfaces/i_goal_deactivator.hpp"
#include "../interfaces/i_unset_goal_expr.hpp"

struct goal_deactivator : i_goal_deactivator {
    goal_deactivator(i_unset_goal_expr& unset_goal_expr);
    void deactivate(const goal_lineage*) override;
private:
    i_unset_goal_expr& unset_goal_expr;
};

#endif
