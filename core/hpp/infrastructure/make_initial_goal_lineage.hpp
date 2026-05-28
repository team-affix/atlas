#ifndef MAKE_INITIAL_GOAL_LINEAGE_HPP
#define MAKE_INITIAL_GOAL_LINEAGE_HPP

#include "../interfaces/i_make_initial_goal_lineage.hpp"
#include "../interfaces/i_make_goal_lineage.hpp"

struct make_initial_goal_lineage : i_make_initial_goal_lineage {
    explicit make_initial_goal_lineage(i_make_goal_lineage& make_goal_lineage);
    const goal_lineage* make(subgoal_id idx) override;
private:
    i_make_goal_lineage& make_goal_lineage;
};

#endif
