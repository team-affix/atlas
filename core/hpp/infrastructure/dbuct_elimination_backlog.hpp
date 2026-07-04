#ifndef DBUCT_ELIMINATION_BACKLOG_HPP
#define DBUCT_ELIMINATION_BACKLOG_HPP

#include <unordered_map>
#include <unordered_set>
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

// Delayed-backtracking variant of elimination_backlog.
//
// The production elimination_backlog routes its mutations through the global
// trail. Under DBUCT the whole per-sim state (this backlog included) is captured
// by the checkpoint machinery, so a plain map with snapshot()/restore() is both
// simpler and rolls back exactly with every other structure at a choice boundary.
struct dbuct_elimination_backlog {
    using eliminated_candidates_type =
        std::unordered_map<const goal_lineage*, std::unordered_set<rule_id>>;
    using snapshot_t = eliminated_candidates_type;

    void insert_backlogged_elimination(const resolution_lineage* rl) {
        eliminated_candidates_[rl->parent].insert(rl->idx);
    }

    bool is_backlogged_elimination(const resolution_lineage* rl) const {
        auto it = eliminated_candidates_.find(rl->parent);
        if (it == eliminated_candidates_.end()) return false;
        return it->second.contains(rl->idx);
    }

    snapshot_t snapshot() const { return eliminated_candidates_; }
    void restore(snapshot_t s) { eliminated_candidates_ = std::move(s); }

private:
    eliminated_candidates_type eliminated_candidates_;
};

#endif
