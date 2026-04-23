#include "../hpp/cdcl_watch.hpp"

cdcl_watch::cdcl_watch(cdcl& c, lineage_pool& lp) : c(c), lp(lp) {}

void cdcl_watch::goal_added(const goal_lineage* gl) {
    frontier_goals.insert(gl);
}

void cdcl_watch::goal_resolved(const goal_lineage* gl) {
    frontier_goals.erase(gl);
}

void cdcl_watch::pipe() {
    auto& new_eliminated_resolutions = c.new_eliminated_resolutions;
    
    // fill the elimination backlog with the new eliminated resolutions
    while (!new_eliminated_resolutions.empty()) {
        // get the next eliminated resolution
        const resolution_lineage* rl = new_eliminated_resolutions.front();
        new_eliminated_resolutions.pop();
        
        // push the resolution to the elimination backlog
        elimination_backlog[rl->parent].insert(rl->idx);
    }

    // process the elimination backlog according to the goals in the frontier
    for (const goal_lineage* gl : frontier_goals) {
        auto& backlog = elimination_backlog[gl];
        for (size_t idx : backlog)
            eliminated_frontier_resolutions.push(lp.resolution(gl, idx));
        backlog.clear();
    }
}