#include "../../hpp/infrastructure/goal_activator.hpp"

goal_activator::goal_activator(
    i_set_goal_expr& set_goal_expr,
    i_get_candidate_translation_map& get_candidate_translation_map,
    i_copier& copier)
    :
    set_goal_expr(set_goal_expr),
    get_candidate_translation_map(get_candidate_translation_map),
    copier(copier) {}

void goal_activator::activate(const goal_lineage* gl) {
    const resolution_lineage* rl = gl->parent;
    translation_map& tm = get_candidate_translation_map.get(rl);
    const expr* copy = copier.copy(gl->idx, tm);
    set_goal_expr.set(gl, copy);
}
