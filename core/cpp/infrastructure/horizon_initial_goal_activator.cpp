#include "infrastructure/horizon_initial_goal_activator.hpp"

horizon_initial_goal_activator::horizon_initial_goal_activator(locator& loc)
    : initial_goal_activator_(loc.locate<initial_goal_activator>()),
      make_initial_goal_lineage_(loc.locate<i_make_initial_goal_lineage>()),
      set_goal_weight_(loc.locate<i_set_goal_weight>()),
      get_initial_goal_weight_(loc.locate<i_get_initial_goal_weight>()) {}

void horizon_initial_goal_activator::activate_initial_goal(subgoal_id idx) {
    initial_goal_activator_.activate_initial_goal(idx);
    const goal_lineage* gl = make_initial_goal_lineage_.make(idx);
    set_goal_weight_.set(gl, get_initial_goal_weight_.get());
}
