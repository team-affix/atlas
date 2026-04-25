#include "../hpp/cdcl_eliminator.hpp"

cdcl_eliminator::cdcl_eliminator(
    const database& db,
    const goals& goals,
    expr_pool& ep,
    candidate_store& cs,
    lineage_pool& lp,
    cdcl& c,
    std::queue<const resolution_lineage*>& unit_queue
) :
    lp(lp),
    cs(cs),
    unit_queue(unit_queue),
    fw(db, lp),
    conflict_register(false) {
    c.set_new_eliminated_resolution_callback(new_eliminated_resolution_callback());
    fw.set_insert_callback(goal_inserted_callback());
    fw.set_resolve_callback(goal_resolved_callback());
    fw.initialize(goals);
    // restore pre-existing eliminations from the cdcl
    for (const resolution_lineage* rl : c.get_eliminated_resolutions())
        route_elimination(rl);
}

bool cdcl_eliminator::operator()() {

    // fill the elimination backlog with the new eliminated resolutions
    while (!conflict_register && !new_eliminated_resolutions.empty()) {
        // get the next eliminated resolution
        const resolution_lineage* rl = new_eliminated_resolutions.front();
        new_eliminated_resolutions.pop();
        
        // route the elimination properly
        route_elimination(rl);
    }

    // return whether a conflict was found
    return conflict_register;
}

void cdcl_eliminator::resolve(const resolution_lineage* r) {
    fw.resolve(r);
}

void cdcl_eliminator::route_elimination(const resolution_lineage* rl) {
    // get the parent goal
    const goal_lineage* gl = rl->parent;

    // if the goal is resolved, do nothing
    if (resolved_goals.contains(gl))
        return;
    
    // if the goal not in the frontier, add to backlog and return
    if (!active_goals.contains(gl)) {
        elimination_backlog[gl].insert(rl->idx);
        return;
    }

    // if the goal is in the frontier, eliminate right now in candidate_store
    eliminate(gl, rl->idx);
}

void cdcl_eliminator::flush_backlog_for_goal(const goal_lineage* gl) {
    auto node = elimination_backlog.extract(gl);

    // if the goal is not in the elimination backlog, do nothing
    if (node.empty())
        return;

    // for each index in the backlog, eliminate the candidate
    // NOTE: we do not have to check for conflict during loop since
    //     we are removing candidates from a single goal, and a conflict
    //     will only arise at the end of the loop anyway if one does (since
    //     it requires candidates.size() == 0)
    for (size_t idx : node.mapped())
        eliminate(gl, idx);
}

void cdcl_eliminator::eliminate(const goal_lineage* gl, size_t idx) {
    // get the candidates for the goal
    auto& candidates = cs.at(gl);

    // get if the goal WAS unit
    bool was_unit = candidates.size() == 1;
    
    // eliminate the candidate
    candidates.erase(idx);

    // if newly unit, push the resolution to the unit queue
    if (!was_unit && candidates.size() == 1)
        mark_unit(gl);

    // if the goal has no candidates, return conflict
    if (candidates.empty())
        conflict_register = true;
}

void cdcl_eliminator::mark_unit(const goal_lineage* gl) {
    const auto& candidates = cs.at(gl);
    unit_queue.push(lp.resolution(gl, *candidates.begin()));
}

std::function<void(const resolution_lineage*)> cdcl_eliminator::new_eliminated_resolution_callback() {
    return [this](const resolution_lineage* rl) {
        new_eliminated_resolutions.push(rl);
    };
}

std::function<void(const goal_lineage*)> cdcl_eliminator::goal_inserted_callback() {
    return [this](const goal_lineage* gl) {
        if (cs.at(gl).size() == 1)
            mark_unit(gl);
        active_goals.insert(gl);
        flush_backlog_for_goal(gl);
    };
}

std::function<void(const resolution_lineage*)> cdcl_eliminator::goal_resolved_callback() {
    return [this](const resolution_lineage* rl) {
        active_goals.erase(rl->parent);
        resolved_goals.insert(rl->parent);
    };
}
