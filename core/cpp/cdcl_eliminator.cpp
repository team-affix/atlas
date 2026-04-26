#include "../hpp/cdcl_eliminator.hpp"

cdcl_eliminator::cdcl_eliminator() :
    lp(locator::locate<lineage_pool>(locator_keys::inst_lineage_pool)),
    cs(locator::locate<candidate_store>(locator_keys::inst_candidate_store)),
    goal_inserted_subscription(locator::locate<topic<const goal_lineage*>>(locator_keys::inst_goal_inserted_topic)),
    goal_resolved_subscription(locator::locate<topic<const resolution_lineage*>>(locator_keys::inst_goal_resolved_topic)),
    new_eliminated_resolution_subscription(locator::locate<topic<const resolution_lineage*>>(locator_keys::inst_new_eliminated_resolution_topic)),
    unit_topic(locator::locate<topic<const resolution_lineage*>>(locator_keys::inst_unit_topic)) {
}

bool cdcl_eliminator::operator()() {
    flush_goal_resolved();
    if (flush_goal_inserted())
        return true;
    if (flush_new_eliminated_resolutions())
        return true;
    return false;
}

bool cdcl_eliminator::route_elimination(const resolution_lineage* rl) {
    // get the parent goal
    const goal_lineage* gl = rl->parent;

    // if the goal is resolved, do nothing
    if (resolved_goals.contains(gl))
        return false;
    
    // if the goal not in the frontier, add to backlog and return
    if (!active_goals.contains(gl)) {
        elimination_backlog[gl].insert(rl->idx);
        return false;
    }

    // if the goal is in the frontier, eliminate right now in candidate_store
    return eliminate(gl, rl->idx);
}

bool cdcl_eliminator::flush_backlog_for_goal(const goal_lineage* gl) {
    auto node = elimination_backlog.extract(gl);

    // if the goal is not in the elimination backlog, do nothing
    if (node.empty())
        return false;

    // for each index in the backlog, eliminate the candidate
    // NOTE: we do not have to check for conflict during loop since
    //     we are removing candidates from a single goal, and a conflict
    //     will only arise at the end of the loop anyway if one does (since
    //     it requires candidates.size() == 0)
    for (size_t idx : node.mapped()) {
        if (eliminate(gl, idx))
            return true;
    }

    return false;
}

bool cdcl_eliminator::eliminate(const goal_lineage* gl, size_t idx) {
    // get the candidates for the goal
    auto& candidates = cs.at(gl);

    // get if the goal WAS unit
    bool was_unit = candidates.size() == 1;
    
    // eliminate the candidate
    candidates.erase(idx);

    // if newly unit, push the resolution to the unit queue
    if (!was_unit && candidates.size() == 1)
        unit_topic.produce(lp.resolution(gl, *candidates.begin()));

    // if the goal has no candidates, return conflict
    return candidates.empty();
}

bool cdcl_eliminator::flush_new_eliminated_resolutions() {
    // fill the elimination backlog with the new eliminated resolutions
    while (!new_eliminated_resolution_subscription.empty()) {
        // get the next eliminated resolution
        const resolution_lineage* rl = new_eliminated_resolution_subscription.consume();
        
        // route the elimination properly
        if (route_elimination(rl))
            return true;
    }
    return false;
}

bool cdcl_eliminator::flush_goal_inserted() {
    while (!goal_inserted_subscription.empty()) {
        const goal_lineage* gl = goal_inserted_subscription.consume();
        active_goals.insert(gl);
        if (flush_backlog_for_goal(gl))
            return true;
    }
    return false;
}

void cdcl_eliminator::flush_goal_resolved() {
    while (!goal_resolved_subscription.empty()) {
        const resolution_lineage* rl = goal_resolved_subscription.consume();
        active_goals.erase(rl->parent);
        resolved_goals.insert(rl->parent);
    }
}
