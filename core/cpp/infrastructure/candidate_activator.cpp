#include "infrastructure/candidate_activator.hpp"

candidate_activator::candidate_activator(locator& loc)
    :
    frame_allocator(loc.locate<i_frame_allocator>()),
    set_candidate_frame_offset(loc.locate<i_set_candidate_frame_offset>()),
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

    const uint32_t frame_offset = frame_allocator.bump(r->var_count);
    const framed_expr goal = get_goal_expr.get(gl);
    const framed_expr head{r->head, frame_offset};

    if (!try_add_mhu_head.try_add_head(rl, goal, head))
        return;

    set_candidate_frame_offset.set(rl, frame_offset);
    link_goal_candidate.link_goal_candidate(gl, rl->idx);
}
