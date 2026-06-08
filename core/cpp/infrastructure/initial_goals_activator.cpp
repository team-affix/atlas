#include "infrastructure/initial_goals_activator.hpp"

initial_goals_activator::initial_goals_activator(locator& loc)
    :
    get_initial_goal_count(loc.locate<i_get_initial_goal_count>()),
    activate_initial_goal(loc.locate<i_activate_initial_goal>()),
    make_initial_goal_lineage(loc.locate<i_make_initial_goal_lineage>()),
    activate_goal_candidates(loc.locate<i_activate_goal_candidates>()) {}

bool initial_goals_activator::activate_initial_goals_and_candidates() {
    for (size_t i = 0; i < get_initial_goal_count.count(); ++i) {
        activate_initial_goal.activate_initial_goal(i);
        const goal_lineage* gl = make_initial_goal_lineage.make(i);
        if (!activate_goal_candidates.activate_goal_candidates(gl))
            return false;
    }
    return true;
}
