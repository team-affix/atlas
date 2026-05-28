#include "infrastructure/elimination_router.hpp"

elimination_router::elimination_router(locator& loc)
    :
    deactivated_candidate_memory(loc.locate<i_deactivated_candidate_memory>()),
    is_active_goal(loc.locate<i_is_active_goal>()),
    insert_backlogged_elimination(loc.locate<i_insert_backlogged_elimination>()),
    candidate_deactivator(loc.locate<i_candidate_deactivator>()) {}

elimination_result elimination_router::route(const resolution_lineage* rl) {
    if (deactivated_candidate_memory.contains(rl))
        return elimination_result::already_deactivated;
    if (!is_active_goal.is_active_goal(rl->parent)) {
        insert_backlogged_elimination.insert_backlogged_elimination(rl);
        return elimination_result::added_to_backlog;
    }
    candidate_deactivator.deactivate(rl);
    return elimination_result::eliminated;
}
