#ifndef ELIMINATION_BACKLOG_HPP
#define ELIMINATION_BACKLOG_HPP

#include <unordered_map>
#include <unordered_set>
#include "infrastructure/tracked.hpp"
#include "infrastructure/trail.hpp"
#include "infrastructure/backtrackable_map_insert.hpp"
#include "infrastructure/backtrackable_map_at_insert.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

struct elimination_backlog {
    explicit elimination_backlog(trail&);
    void insert_backlogged_elimination(const resolution_lineage*);
    bool is_backlogged_elimination(const resolution_lineage*) const;
private:
    using eliminated_candidates_type =
        std::unordered_map<const goal_lineage*, std::unordered_set<rule_id>>;
    tracked<eliminated_candidates_type, trail> eliminated_candidates;
};

inline elimination_backlog::elimination_backlog(trail& t)
    : eliminated_candidates(t, {}) {}

inline void elimination_backlog::insert_backlogged_elimination(const resolution_lineage* rl) {
    const goal_lineage* gl = rl->parent;
    if (!eliminated_candidates.get().contains(gl)) {
        auto insert_mut = std::make_unique<
            backtrackable_map_insert<eliminated_candidates_type>>(
                gl, std::unordered_set<rule_id>{});
        eliminated_candidates.mutate(std::move(insert_mut));
    }
    if (!eliminated_candidates.get().at(gl).contains(rl->idx)) {
        auto insert_mut = std::make_unique<
            backtrackable_map_at_insert<eliminated_candidates_type>>(
                gl, rl->idx);
        eliminated_candidates.mutate(std::move(insert_mut));
    }
}

inline bool elimination_backlog::is_backlogged_elimination(const resolution_lineage* rl) const {
    auto it = eliminated_candidates.get().find(rl->parent);
    if (it == eliminated_candidates.get().end()) return false;
    return it->second.contains(rl->idx);
}

#endif
