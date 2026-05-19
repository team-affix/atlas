#include "../../hpp/infrastructure/goal_candidate_deactivator_visitor.hpp"

goal_candidate_deactivator_visitor::goal_candidate_deactivator_visitor(
    const goal_lineage* gl,
    i_mhu_elimination_generator& mhu_elimination_generator,
    i_candidate_deactivator& candidate_deactivator,
    i_elimination_backlog& elimination_backlog,
    i_lineage_pool& lp,
    i_deactivate_candidate_translation_map& dctm)
    :
    gl(gl),
    mhu_elimination_generator(mhu_elimination_generator),
    candidate_deactivator(candidate_deactivator),
    elimination_backlog(elimination_backlog),
    lp(lp),
    dctm(dctm) {}

void goal_candidate_deactivator_visitor::visit(const rule* r) {
    const resolution_lineage* rl = lp.resolution(gl, r);

    mhu_elimination_generator.remove_head(rl);
    dctm.deactivate(rl);
    candidate_deactivator.deactivate(rl);
}
