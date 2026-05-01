#include "../../../hpp/domain/entities/elimination_backlog.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

elimination_backlog::elimination_backlog()
    :
    cs(resolver::resolve<i_candidate_store>()),
    active_goal_store(resolver::resolve<i_active_goal_store>()),
    inactive_goal_store(resolver::resolve<i_inactive_goal_store>()) {
}

void elimination_backlog::insert(const resolution_lineage* rl) {
    // get the parent goal
    const goal_lineage* gl = rl->parent;

    // if the goal is resolved, do nothing
    if (inactive_goal_store.contains(gl))
        return;
    
    // if the goal not in the frontier, add to backlog and return
    if (!active_goal_store.contains(gl)) {
        backlog[gl].candidates.insert(rl->idx);
        return;
    }

    // if the goal is in the frontier, eliminate right now in candidate_store
    cs.eliminate(gl, rl->idx);
}

void elimination_backlog::goal_activated(const goal_lineage* gl) {
    auto node = backlog.extract(gl);

    // if the goal is not in the elimination backlog, do nothing
    if (node.empty())
        return;

    // for each index in the backlog, eliminate the candidate
    for (size_t idx : node.mapped().candidates)
        cs.eliminate(gl, idx);
}
