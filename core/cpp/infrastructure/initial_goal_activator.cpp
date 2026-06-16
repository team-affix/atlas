#include "infrastructure/initial_goal_activator.hpp"

initial_goal_activator::initial_goal_activator(locator& loc)
    :
    get_initial_goal_expr(loc.locate<i_get_initial_goal_expr>()),
    make_initial_goal_lineage(loc.locate<i_make_initial_goal_lineage>()),
    set_goal_expr(loc.locate<i_set_goal_expr>()),
    insert_goal_candidates(loc.locate<i_insert_goal_candidates>()),
    insert_active_goal(loc.locate<i_insert_active_goal>()) {}

void initial_goal_activator::activate_initial_goal(subgoal_id idx) {
    const expr* goal_expr = get_initial_goal_expr.get(idx);
    const goal_lineage* gl = make_initial_goal_lineage.make(idx);
    // Initial goals always live at frame offset 0.
    set_goal_expr.set(gl, {goal_expr, 0});
    insert_goal_candidates.insert(gl);
    insert_active_goal.insert_active_goal(gl);
}
