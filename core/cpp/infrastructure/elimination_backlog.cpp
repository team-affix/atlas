#include "../../hpp/infrastructure/elimination_backlog.hpp"

void elimination_backlog::insert_backlogged_elimination(const resolution_lineage* rl) {
    backlogged_.insert(rl);
}

bool elimination_backlog::is_backlogged_elimination(const resolution_lineage* rl) const {
    return backlogged_.contains(rl);
}

void elimination_backlog::constrain_elimination_backlog(const resolution_lineage* rl) {
    const goal_lineage* gl = rl->parent;
    for (auto it = backlogged_.begin(); it != backlogged_.end();) {
        if ((*it)->parent == gl)
            it = backlogged_.erase(it);
        else
            ++it;
    }
}
