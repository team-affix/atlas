#ifndef DBUCT_SRT_ACTIVE_GOALS_HPP
#define DBUCT_SRT_ACTIVE_GOALS_HPP

#include <set>
#include "infrastructure/series_reduced_tree.hpp"
#include "infrastructure/coroutine.hpp"
#include "value_objects/lineage.hpp"
#include "debug_assert.hpp"

// Delayed-backtracking variant of srt_active_goals.
//
// The series-reduced goal tree is built out of standard containers and is
// therefore copyable; a snapshot captures the whole tree plus the in-flight
// batch set, so restoration re-instates the exact active frontier at any choice
// boundary (including the correct branch/leaf structure that MCTS navigates).
struct dbuct_srt_active_goals {
    struct snapshot_t {
        series_reduced_tree<const goal_lineage*> tree;
        std::set<const goal_lineage*> in_flight;
    };

    void insert_active_goal(const goal_lineage* gl);
    void link_srt_goal_batch_parent(const goal_lineage* parent);
    void flush_srt_goal_batch();
    bool is_active_goal(const goal_lineage* gl) const;
    size_t active_goals_size() const;
    bool empty() const;
    coroutine<const goal_lineage*, void> iterate_root_goals() const;
    coroutine<const goal_lineage*, void> iterate_child_goals(const goal_lineage* gl) const;

    snapshot_t snapshot() const;
    void restore(snapshot_t s);

private:
    series_reduced_tree<const goal_lineage*> tree_;
    std::set<const goal_lineage*> in_flight_;
};

inline void dbuct_srt_active_goals::insert_active_goal(const goal_lineage* gl) {
    auto [_, inserted] = in_flight_.emplace(gl);
    DEBUG_ASSERT(inserted);
    const bool tree_inserted = tree_.insert(gl);
    DEBUG_ASSERT(tree_inserted);
}

inline void dbuct_srt_active_goals::link_srt_goal_batch_parent(const goal_lineage* parent) {
    tree_.link(parent, in_flight_);
}

inline void dbuct_srt_active_goals::flush_srt_goal_batch() { in_flight_.clear(); }

inline bool dbuct_srt_active_goals::is_active_goal(const goal_lineage* gl) const {
    return tree_.leaves().contains(gl);
}

inline size_t dbuct_srt_active_goals::active_goals_size() const { return tree_.leaves().size(); }

inline bool dbuct_srt_active_goals::empty() const { return tree_.leaves().empty(); }

inline coroutine<const goal_lineage*, void> dbuct_srt_active_goals::iterate_root_goals() const {
    for (const goal_lineage* gl : tree_.roots()) co_yield gl;
}

inline coroutine<const goal_lineage*, void> dbuct_srt_active_goals::iterate_child_goals(const goal_lineage* gl) const {
    for (const goal_lineage* child : tree_.children(gl)) co_yield child;
}

inline dbuct_srt_active_goals::snapshot_t dbuct_srt_active_goals::snapshot() const { return snapshot_t{tree_, in_flight_}; }
inline void dbuct_srt_active_goals::restore(snapshot_t s) {
    tree_ = std::move(s.tree);
    in_flight_ = std::move(s.in_flight);
}

#endif
