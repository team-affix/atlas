#include "infrastructure/goal_activator.hpp"

goal_activator::goal_activator(locator& loc)
    :
    set_goal_expr(loc.locate<i_set_goal_expr>()),
    insert_goal_candidates(loc.locate<i_insert_goal_candidates>()),
    insert_active_goal(loc.locate<i_insert_active_goal>()),
    get_candidate_translation_map(loc.locate<i_get_candidate_translation_map>()),
    get_resolution_rule(loc.locate<i_get_resolution_rule>()),
    copier(loc.locate<i_copier>()) {}

void goal_activator::activate(const goal_lineage* gl) {
    const resolution_lineage* rl = gl->parent;
    const rule* rule = get_resolution_rule.get(rl);
    translation_map& tm = get_candidate_translation_map.get(rl);
    const expr* body_expr = rule->body.at(gl->idx);
    const expr* copy = copier.copy(body_expr, tm);
    set_goal_expr.set(gl, copy);
    insert_goal_candidates.insert(gl);
    insert_active_goal.insert_active_goal(gl);
}
