#include "infrastructure/subgoals_activator.hpp"

subgoals_activator::subgoals_activator(locator& loc)
    :
    make_goal_lineage(loc.locate<i_make_goal_lineage>()),
    goal_activator(loc.locate<i_goal_activator>()),
    get_rule(loc.locate<i_get_rule>()),
    activate_goal_candidates(loc.locate<i_activate_goal_candidates>()) {}

bool subgoals_activator::activate_subgoals(const resolution_lineage* rl) {
    const rule* rule = get_rule.get(rl->idx);
    for (size_t body_idx = 0; body_idx < rule->body.size(); ++body_idx) {
        const goal_lineage* gl = make_goal_lineage.make_goal_lineage(rl, body_idx);
        goal_activator.activate(gl);
        if (!activate_goal_candidates.activate_goal_candidates(gl))
            return false;
    }
    return true;
}
