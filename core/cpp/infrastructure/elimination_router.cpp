#include "../../hpp/infrastructure/elimination_router.hpp"

elimination_router::elimination_router(
    i_deactivated_candidate_memory& dcm,
    i_active_goals& ag,
    i_elimination_backlog& eb,
    i_candidate_deactivator& cd)
    : dcm(dcm), ag(ag), eb(eb), cd(cd) {
}

elimination_result elimination_router::route(const resolution_lineage* rl) {
    // 1. check if candidate is deactivated
    if (dcm.contains(rl))
        return elimination_result::already_deactivated;
    // 2. check if candidate is not yet active
    if (!ag.contains(rl->parent)) {
        eb.insert(rl);
        return elimination_result::added_to_backlog;
    }
    // 3. the candidate is active, deactivate it
    cd.deactivate(rl);
    return elimination_result::eliminated;
}
