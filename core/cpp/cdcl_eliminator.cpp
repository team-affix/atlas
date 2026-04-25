#include "../hpp/cdcl_eliminator.hpp"

cdcl_eliminator::cdcl_eliminator(
    const database& db,
    const goals& goals,
    expr_pool& ep,
    candidate_store& cs,
    lineage_pool& lp,
    cdcl& c
) : lp(lp), cs(cs), fw(db, lp), flush_produced_conflict(false) {
    c.set_new_eliminated_resolution_callback(new_eliminated_resolution_callback());
    fw.set_insert_callback(goal_inserted_callback());
    fw.set_resolve_callback(goal_resolved_callback());
    fw.initialize(goals);
}

bool cdcl_eliminator::operator()() {
    // if a conflict was produced by the flush, return it
    if (flush_produced_conflict)
        return true;
    
    // fill the elimination backlog with the new eliminated resolutions
    while (!new_eliminated_resolutions.empty()) {
        // get the next eliminated resolution
        const resolution_lineage* rl = new_eliminated_resolutions.front();
        new_eliminated_resolutions.pop();
        
        // route the elimination properly
        if (route_elimination(rl))
            return true;
    }

    // no conflict found
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

    // for each index in the backlog, eliminate the candidate
    for (size_t idx : node.mapped()) {
        if (eliminate(gl, idx))
            return true;
    }

    // no conflict found
    return false;
}

bool cdcl_eliminator::eliminate(const goal_lineage* gl, size_t idx) {
    // get the candidates for the goal
    auto& candidates = cs.at(gl);

    // eliminate the candidate
    candidates.erase(idx);

    // if the goal has no candidates, return conflict
    return candidates.empty();
}

std::function<void(const resolution_lineage*)> cdcl_eliminator::new_eliminated_resolution_callback() {
    return [this](const resolution_lineage* rl) {
        new_eliminated_resolutions.push(rl);
    };
}

std::function<void(const goal_lineage*)> cdcl_eliminator::goal_inserted_callback() {
    return [this](const goal_lineage* gl) {
        active_goals.insert(gl);
        if (flush_backlog_for_goal(gl))
            flush_produced_conflict = true;
    };
}

std::function<void(const resolution_lineage*)> cdcl_eliminator::goal_resolved_callback() {
    return [this](const resolution_lineage* rl) {
        active_goals.erase(rl->parent);
        resolved_goals.insert(rl->parent);
    };
}
