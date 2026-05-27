#include "../../hpp/infrastructure/elimination_router.hpp"

elimination_router::elimination_router(
    i_deactivated_candidate_memory& dcm,
    i_is_active_goal& is_active_goal,
    i_elimination_backlog& eb,
    i_candidate_deactivator& cd)
    : dcm(dcm), is_active_goal(is_active_goal), eb(eb), cd(cd) {
}

elimination_result elimination_router::route(const resolution_lineage* rl) {
    if (dcm.contains(rl))
        return elimination_result::already_deactivated;
    if (!is_active_goal.is_active_goal(rl->parent)) {
        eb.insert(rl);
        return elimination_result::added_to_backlog;
    }
    cd.deactivate(rl);
    return elimination_result::eliminated;
}
