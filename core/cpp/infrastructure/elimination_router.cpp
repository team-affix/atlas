#include "../../hpp/infrastructure/elimination_router.hpp"

elimination_router::elimination_router(
    i_deactivated_candidate_memory& deactivated_candidate_memory,
    i_is_active_goal& is_active_goal,
    i_insert_backlogged_elimination& insert_backlogged_elimination,
    i_candidate_deactivator& candidate_deactivator)
    :
    deactivated_candidate_memory(deactivated_candidate_memory),
    is_active_goal(is_active_goal),
    insert_backlogged_elimination(insert_backlogged_elimination),
    candidate_deactivator(candidate_deactivator) {}

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
