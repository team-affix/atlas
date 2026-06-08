#include "infrastructure/resolver.hpp"

resolver::resolver(locator& loc)
    :
    goal_deactivator(loc.locate<i_goal_deactivator>()),
    activate_subgoals_and_candidates(loc.locate<i_activate_subgoals_and_candidates>()),
    deactivate_goal_candidates(loc.locate<i_deactivate_goal_candidates>()) {}

bool resolver::resolve(const resolution_lineage* rl) {
    if (!activate_subgoals_and_candidates.activate_subgoals_and_candidates(rl))
        return false;
    const goal_lineage* gl = rl->parent;
    deactivate_goal_candidates.deactivate_goal_candidates(gl);
    goal_deactivator.deactivate(gl);
    return true;
}
