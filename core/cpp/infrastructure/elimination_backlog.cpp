#include "../../hpp/infrastructure/elimination_backlog.hpp"

void elimination_backlog::insert_backlogged_elimination(const resolution_lineage* rl) {
    eliminated_candidates_[rl->parent].insert(rl->idx);
}

bool elimination_backlog::is_backlogged_elimination(const resolution_lineage* rl) const {
    auto it = eliminated_candidates_.find(rl->parent);
    if (it == eliminated_candidates_.end())
        return false;
    return it->second.contains(rl->idx);
}

void elimination_backlog::constrain_elimination_backlog(const resolution_lineage* rl) {
    eliminated_candidates_.erase(rl->parent);
}
