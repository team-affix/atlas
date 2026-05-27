#include "../../hpp/infrastructure/candidate_activator.hpp"

candidate_activator::candidate_activator(
    i_copier& copier,
    i_activate_candidate_translation_map& actm,
    i_mhu_elimination_generator& mhu_elimination_generator,
    i_elimination_backlog& elimination_backlog,
    i_get_goal_expr& gge)
    :
    copier(copier),
    actm(actm),
    mhu_elimination_generator(mhu_elimination_generator),
    elimination_backlog(elimination_backlog),
    gge(gge) {}

void candidate_activator::activate(const resolution_lineage* rl) {
    const goal_lineage* gl = rl->parent;
    rule_id r = rl->idx;

    if (elimination_backlog.contains(rl))
        return;

    translation_map tm;
    const expr* copied_head = copier.copy(r->head, tm);

    const expr* goal_expr = gge.get(gl);

    if (!mhu_elimination_generator.try_add_head(rl, goal_expr, copied_head))
        return;

    actm.activate(rl, std::move(tm));
}
