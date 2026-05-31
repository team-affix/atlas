#include "infrastructure/elimination_router.hpp"

elimination_router::elimination_router(locator& loc)
    :
    get_goal_candidate_rule_ids(loc.locate<i_get_goal_candidate_rule_ids>()),
    is_active_goal(loc.locate<i_is_active_goal>()),
    insert_backlogged_elimination(loc.locate<i_insert_backlogged_elimination>()),
    candidate_deactivator(loc.locate<i_candidate_deactivator>()) {}

elimination_result elimination_router::route(const resolution_lineage* rl) {
    if (!is_active_goal.is_active_goal(rl->parent)) {
        insert_backlogged_elimination.insert_backlogged_elimination(rl);
        return elimination_result::added_to_backlog;
    }
    if (!get_goal_candidate_rule_ids.get(rl->parent).contains(rl->idx))
        return elimination_result::already_deactivated;
    candidate_deactivator.deactivate(rl);
    return elimination_result::eliminated;
}
