#include "../../../hpp/domain/entities/elimination_router.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

elimination_router::elimination_router()
    : active_goal_store(resolver::resolve<i_active_goal_store>()),
    inactive_goal_store(resolver::resolve<i_inactive_goal_store>()),
    elimination_backlog(resolver::resolve<i_elimination_backlog>()),
    active_eliminator(resolver::resolve<i_active_eliminator>()) {
}

void elimination_router::route(const resolution_lineage* rl) {
    // get the parent goal
    const goal_lineage* gl = rl->parent;

    // if the goal is resolved, do nothing
    if (inactive_goal_store.contains(gl))
        return;
    
    // if the goal not in the frontier, add to backlog and return
    if (!active_goal_store.contains(gl)) {
        elimination_backlog.insert(rl);
        return;
    }

    // if the goal is in the frontier, eliminate right now in candidate_store
    active_eliminator.eliminate(gl, rl->idx);
}