#include "infrastructure/goal_activator.hpp"

goal_activator::goal_activator(
    i_set_goal_expr& set_goal_expr,
    i_get_candidate_translation_map& get_candidate_translation_map,
    i_get_resolution_rule& get_resolution_rule,
    i_copier& copier)
    :
    set_goal_expr(set_goal_expr),
    get_candidate_translation_map(get_candidate_translation_map),
    get_resolution_rule(get_resolution_rule),
    copier(copier) {}

void goal_activator::activate(const goal_lineage* gl) {
    const resolution_lineage* rl = gl->parent;
    const rule* rule = get_resolution_rule.get(rl);
    translation_map& tm = get_candidate_translation_map.get(rl);
    const expr* body_expr = rule->body.at(gl->idx);
    const expr* copy = copier.copy(body_expr, tm);
    set_goal_expr.set(gl, copy);
}
