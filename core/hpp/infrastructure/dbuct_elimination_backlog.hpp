#ifndef DBUCT_ELIMINATION_BACKLOG_HPP
#define DBUCT_ELIMINATION_BACKLOG_HPP

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include "infrastructure/backtrackable_map_at_insert.hpp"
#include "infrastructure/backtrackable_map_insert.hpp"
#include "infrastructure/tracked.hpp"
#include "infrastructure/trail.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

// Delayed-backtracking variant of elimination_backlog.
//
// Like the production elimination_backlog, this routes its mutations through the
// shared trail (rather than a full-copy snapshot): a new goal key logs a map
// insert and each eliminated candidate logs an inner-set insert, so a choice
// frame rolls the backlog back exactly on pop.
struct dbuct_elimination_backlog {
    using eliminated_candidates_type =
        std::unordered_map<const goal_lineage*, std::unordered_set<rule_id>>;

    explicit dbuct_elimination_backlog(trail& t);

    void insert_backlogged_elimination(const resolution_lineage* rl);
    bool is_backlogged_elimination(const resolution_lineage* rl) const;

private:
    tracked<eliminated_candidates_type, trail> eliminated_candidates_;
};

inline dbuct_elimination_backlog::dbuct_elimination_backlog(trail& t) : eliminated_candidates_(t, eliminated_candidates_type{}) {}

inline void dbuct_elimination_backlog::insert_backlogged_elimination(const resolution_lineage* rl) {
    const goal_lineage* gl = rl->parent;
    if (!eliminated_candidates_.get().contains(gl))
        eliminated_candidates_.mutate(
            std::make_unique<backtrackable_map_insert<eliminated_candidates_type>>(gl, std::unordered_set<rule_id>{}));
    if (!eliminated_candidates_.get().at(gl).contains(rl->idx))
        eliminated_candidates_.mutate(
            std::make_unique<backtrackable_map_at_insert<eliminated_candidates_type>>(gl, rl->idx));
}

inline bool dbuct_elimination_backlog::is_backlogged_elimination(const resolution_lineage* rl) const {
    const auto& m = eliminated_candidates_.get();
    auto it = m.find(rl->parent);
    if (it == m.end()) return false;
    return it->second.contains(rl->idx);
}

#endif
