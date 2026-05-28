#include "infrastructure/initial_goal_activator.hpp"

initial_goal_activator::initial_goal_activator(
    i_get_initial_goal_expr& get_initial_goal_expr,
    i_make_initial_goal_lineage& make_initial_goal_lineage,
    i_set_goal_expr& set_goal_expr,
    i_insert_active_goal& insert_active_goal)
    :
    get_initial_goal_expr(get_initial_goal_expr),
    make_initial_goal_lineage(make_initial_goal_lineage),
    set_goal_expr(set_goal_expr),
    insert_active_goal(insert_active_goal) {}

void initial_goal_activator::activate_initial_goal(subgoal_id idx) {
    const expr* goal_expr = get_initial_goal_expr.get(idx);
    const goal_lineage* gl = make_initial_goal_lineage.make(idx);
    set_goal_expr.set(gl, goal_expr);
    insert_active_goal.insert_active_goal(gl);
}
