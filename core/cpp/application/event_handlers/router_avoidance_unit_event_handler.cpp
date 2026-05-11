#include "../../../hpp/application/event_handlers/router_avoidance_unit_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

router_avoidance_unit_event_handler::router_avoidance_unit_event_handler() :
    c(locator::locate<i_cdcl>()),
    active_goal_store(locator::locate<i_active_goal_store>()),
    inactive_goal_store(locator::locate<i_inactive_goal_store>()),
    elimination_backlog(locator::locate<i_elimination_backlog>()),
    active_eliminator(locator::locate<i_active_eliminator>()) {
}

void router_avoidance_unit_event_handler::execute(const avoidance_unit_event& e) {
    // get the avoidance
    const avoidance& av = c.get_avoidance(e.avoidance_id);
    
    // get the first resolution
    const resolution_lineage* rl = *av.decisions.begin();
    
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
    active_eliminator.eliminate(rl);
}
