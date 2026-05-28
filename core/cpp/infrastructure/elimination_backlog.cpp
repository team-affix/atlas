#include <memory>
#include "infrastructure/elimination_backlog.hpp"
#include "infrastructure/backtrackable_map_insert.hpp"
#include "infrastructure/backtrackable_map_at_insert.hpp"

elimination_backlog::elimination_backlog(locator& loc) :
    eliminated_candidates(loc.locate<i_log_to_current_trail_frame>(), {}) {
}

void elimination_backlog::insert_backlogged_elimination(const resolution_lineage* rl) {
    const goal_lineage* gl = rl->parent;

    if (!eliminated_candidates.get().contains(gl)) {
        auto insert_mut = std::make_unique<
            backtrackable_map_insert<
            eliminated_candidates_type>>(
                gl, std::unordered_set<rule_id>{});
        eliminated_candidates.mutate(std::move(insert_mut));
    }

    if (!eliminated_candidates.get().at(gl).contains(rl->idx)) {
        auto insert_mut = std::make_unique<
            backtrackable_map_at_insert<
            eliminated_candidates_type>>(
                gl, rl->idx);
        eliminated_candidates.mutate(std::move(insert_mut));
    }
}

bool elimination_backlog::is_backlogged_elimination(const resolution_lineage* rl) const {
    auto it = eliminated_candidates.get().find(rl->parent);
    if (it == eliminated_candidates.get().end())
        return false;
    return it->second.contains(rl->idx);
}
