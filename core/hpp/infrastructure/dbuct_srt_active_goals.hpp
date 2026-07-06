#ifndef DBUCT_SRT_ACTIVE_GOALS_HPP
#define DBUCT_SRT_ACTIVE_GOALS_HPP

#include <memory>
#include <set>
#include "infrastructure/backtrackable_set_clear.hpp"
#include "infrastructure/backtrackable_set_insert.hpp"
#include "infrastructure/coroutine.hpp"
#include "infrastructure/dbuct_series_reduced_tree.hpp"
#include "infrastructure/tracked.hpp"
#include "infrastructure/trail.hpp"
#include "value_objects/lineage.hpp"
#include "debug_assert.hpp"

// Delayed-backtracking variant of srt_active_goals.
//
// Wraps a trail-journalled series-reduced goal tree plus a trail-journalled
// in-flight batch set. Instead of copying the whole tree per choice, the active
// frontier (with its exact branch/leaf structure that MCTS navigates) is rolled
// back incrementally when the trail pops. flush logs a set-clear whose undo
// restores the batch.
struct dbuct_srt_active_goals {
    using in_flight_t = std::set<const goal_lineage*>;

    explicit dbuct_srt_active_goals(trail& t);

    void insert_active_goal(const goal_lineage* gl);
    void link_srt_goal_batch_parent(const goal_lineage* parent);
    void flush_srt_goal_batch();
    bool is_active_goal(const goal_lineage* gl) const;
    size_t active_goals_size() const;
    bool empty() const;
    coroutine<const goal_lineage*, void> iterate_root_goals() const;
    coroutine<const goal_lineage*, void> iterate_child_goals(const goal_lineage* gl) const;

private:
    dbuct_series_reduced_tree<const goal_lineage*> tree_;
    tracked<in_flight_t, trail> in_flight_;
};

inline dbuct_srt_active_goals::dbuct_srt_active_goals(trail& t) : tree_(t), in_flight_(t, in_flight_t{}) {}

inline void dbuct_srt_active_goals::insert_active_goal(const goal_lineage* gl) {
    in_flight_.mutate(std::make_unique<backtrackable_set_insert<in_flight_t>>(gl));
    const bool tree_inserted = tree_.insert(gl);
    DEBUG_ASSERT(tree_inserted);
}

inline void dbuct_srt_active_goals::link_srt_goal_batch_parent(const goal_lineage* parent) {
    tree_.link(parent, in_flight_.get());
}

inline void dbuct_srt_active_goals::flush_srt_goal_batch() {
    in_flight_.mutate(std::make_unique<backtrackable_set_clear<in_flight_t>>());
}

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

#endif
