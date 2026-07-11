#ifndef MAKE_INITIAL_GOAL_LINEAGE_HPP
#define MAKE_INITIAL_GOAL_LINEAGE_HPP

#include "value_objects/lineage.hpp"

template<typename IMakeGoalLineage>
struct make_initial_goal_lineage {
    make_initial_goal_lineage(IMakeGoalLineage& lp);
    const goal_lineage* make(subgoal_id idx);
private:
    IMakeGoalLineage& make_goal_lineage;
};

template<typename IMakeGoalLineage>
make_initial_goal_lineage<IMakeGoalLineage>::make_initial_goal_lineage(IMakeGoalLineage& lp)
    : make_goal_lineage(lp) {}

template<typename IMakeGoalLineage>
const goal_lineage* make_initial_goal_lineage<IMakeGoalLineage>::make(subgoal_id idx) {
    return make_goal_lineage.make_goal_lineage(nullptr, idx);
}

#endif
