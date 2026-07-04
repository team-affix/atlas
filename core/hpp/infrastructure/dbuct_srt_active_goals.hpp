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

    void insert_active_goal(const goal_lineage* gl) {
        auto [_, inserted] = in_flight_.emplace(gl);
        DEBUG_ASSERT(inserted);
        const bool tree_inserted = tree_.insert(gl);
        DEBUG_ASSERT(tree_inserted);
    }

    void link_srt_goal_batch_parent(const goal_lineage* parent) {
        tree_.link(parent, in_flight_);
    }

    void flush_srt_goal_batch() { in_flight_.clear(); }

    bool is_active_goal(const goal_lineage* gl) const {
        return tree_.leaves().contains(gl);
    }

    size_t active_goals_size() const { return tree_.leaves().size(); }

    bool empty() const { return tree_.leaves().empty(); }

    void clear_active_goals() {
        tree_.clear();
        in_flight_.clear();
    }

    coroutine<const goal_lineage*, void> iterate_root_goals() const {
        for (const goal_lineage* gl : tree_.roots()) co_yield gl;
    }

    coroutine<const goal_lineage*, void> iterate_child_goals(const goal_lineage* gl) const {
        for (const goal_lineage* child : tree_.children(gl)) co_yield child;
    }

    snapshot_t snapshot() const { return snapshot_t{tree_, in_flight_}; }
    void restore(snapshot_t s) {
        tree_ = std::move(s.tree);
        in_flight_ = std::move(s.in_flight);
    }

private:
    series_reduced_tree<const goal_lineage*> tree_;
    std::set<const goal_lineage*> in_flight_;
};

#endif
