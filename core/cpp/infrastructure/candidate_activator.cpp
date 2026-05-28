#include "infrastructure/candidate_activator.hpp"

candidate_activator::candidate_activator(locator& loc)
    :
    copier(loc.locate<i_copier>()),
    set_candidate_translation_map(loc.locate<i_set_candidate_translation_map>()),
    try_add_mhu_head(loc.locate<i_try_add_mhu_head>()),
    is_backlogged_elimination(loc.locate<i_is_backlogged_elimination>()),
    get_goal_expr(loc.locate<i_get_goal_expr>()),
    get_rule(loc.locate<i_get_rule>()),
    link_goal_candidate(loc.locate<i_link_goal_candidate>()) {}

void candidate_activator::activate(const resolution_lineage* rl) {
    const goal_lineage* gl = rl->parent;
    const rule* r = get_rule.get(rl->idx);

    if (is_backlogged_elimination.is_backlogged_elimination(rl))
        return;

    translation_map tm;
    const expr* copied_head = copier.copy(r->head, tm);

    const expr* goal_expr = get_goal_expr.get(gl);

    if (!try_add_mhu_head.try_add_head(rl, goal_expr, copied_head))
        return;

    set_candidate_translation_map.set(rl, std::move(tm));
    link_goal_candidate.link_goal_candidate(gl, rl->idx);
}
