#ifndef GOAL_DEACTIVATOR_HPP
#define GOAL_DEACTIVATOR_HPP

#include "infrastructure/locator.hpp"
#include "interfaces/i_goal_deactivator.hpp"
#include "interfaces/i_unset_goal_expr.hpp"
#include "interfaces/i_erase_goal_candidates.hpp"
#include "interfaces/i_erase_active_goal.hpp"

struct goal_deactivator : i_goal_deactivator {
    goal_deactivator(locator& loc);
    void deactivate(const goal_lineage*) override;
private:
    i_unset_goal_expr& unset_goal_expr;
    i_erase_goal_candidates& erase_goal_candidates;
    i_erase_active_goal& erase_active_goal;
};

#endif
