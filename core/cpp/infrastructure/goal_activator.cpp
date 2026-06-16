#include "infrastructure/goal_activator.hpp"

goal_activator::goal_activator(locator& loc)
    :
    set_goal_expr(loc.locate<i_set_goal_expr>()),
    insert_goal_candidates(loc.locate<i_insert_goal_candidates>()),
    insert_active_goal(loc.locate<i_insert_active_goal>()),
    get_candidate_frame_offset(loc.locate<i_get_candidate_frame_offset>()),
    get_resolution_rule(loc.locate<i_get_resolution_rule>()) {}

void goal_activator::activate(const goal_lineage* gl) {
    const resolution_lineage* rl = gl->parent;
    const rule* r = get_resolution_rule.get(rl);
    const uint32_t frame_offset = get_candidate_frame_offset.get(rl);
    const expr* body_expr = r->body.at(gl->idx);
    set_goal_expr.set(gl, {body_expr, frame_offset});
    insert_goal_candidates.insert(gl);
    insert_active_goal.insert_active_goal(gl);
}
