#include "infrastructure/make_initial_goal_lineage.hpp"

make_initial_goal_lineage::make_initial_goal_lineage(locator& loc)
    : make_goal_lineage(loc.locate<i_make_goal_lineage>()) {}

const goal_lineage* make_initial_goal_lineage::make(subgoal_id idx) {
    return make_goal_lineage.make_goal_lineage(nullptr, idx);
}
