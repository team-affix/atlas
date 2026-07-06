#ifndef DBUCT_SRT_ACTIVE_GOALS_HPP
#define DBUCT_SRT_ACTIVE_GOALS_HPP

#include <memory>
#include <set>
#include "infrastructure/backtrackable_set_clear.hpp"
#include "infrastructure/backtrackable_set_insert.hpp"
#include "infrastructure/coroutine.hpp"
#include "infrastructure/dbuct_series_reduced_tree.hpp"
#include "infrastructure/tracked.hpp"
#include "value_objects/lineage.hpp"
#include "debug_assert.hpp"

// Delayed-backtracking variant of srt_active_goals. Wraps a trail-journalled
// series-reduced goal tree plus a trail-journalled in-flight batch set (trail
// abstracted as ILogTrailAction), so the active frontier MCTS navigates is rolled
// back incrementally on trail pop instead of copied per choice.
template<typename ILogTrailAction>
struct dbuct_srt_active_goals {
    using in_flight_t = std::set<const goal_lineage*>;

    explicit dbuct_srt_active_goals(ILogTrailAction& t);

    void insert_active_goal(const goal_lineage* gl);
    void link_srt_goal_batch_parent(const goal_lineage* parent);
    void flush_srt_goal_batch();
    bool is_active_goal(const goal_lineage* gl) const;
    size_t active_goals_size() const;
    bool empty() const;
    coroutine<const goal_lineage*, void> iterate_root_goals() const;
    coroutine<const goal_lineage*, void> iterate_child_goals(const goal_lineage* gl) const;

private:
    dbuct_series_reduced_tree<const goal_lineage*, ILogTrailAction> tree_;
    tracked<in_flight_t, ILogTrailAction> in_flight_;
};

template<typename ILogTrailAction>
dbuct_srt_active_goals<ILogTrailAction>::dbuct_srt_active_goals(ILogTrailAction& t) : tree_(t), in_flight_(t, in_flight_t{}) {}

template<typename ILogTrailAction>
void dbuct_srt_active_goals<ILogTrailAction>::insert_active_goal(const goal_lineage* gl) {
    in_flight_.mutate(std::make_unique<backtrackable_set_insert<in_flight_t>>(gl));
    const bool tree_inserted = tree_.insert(gl);
    DEBUG_ASSERT(tree_inserted);
}

template<typename ILogTrailAction>
void dbuct_srt_active_goals<ILogTrailAction>::link_srt_goal_batch_parent(const goal_lineage* parent) {
    tree_.link(parent, in_flight_.get());
}

template<typename ILogTrailAction>
void dbuct_srt_active_goals<ILogTrailAction>::flush_srt_goal_batch() {
    in_flight_.mutate(std::make_unique<backtrackable_set_clear<in_flight_t>>());
}

template<typename ILogTrailAction>
bool dbuct_srt_active_goals<ILogTrailAction>::is_active_goal(const goal_lineage* gl) const {
    return tree_.leaves().contains(gl);
}

template<typename ILogTrailAction>
size_t dbuct_srt_active_goals<ILogTrailAction>::active_goals_size() const { return tree_.leaves().size(); }

template<typename ILogTrailAction>
bool dbuct_srt_active_goals<ILogTrailAction>::empty() const { return tree_.leaves().empty(); }

template<typename ILogTrailAction>
coroutine<const goal_lineage*, void> dbuct_srt_active_goals<ILogTrailAction>::iterate_root_goals() const {
    for (const goal_lineage* gl : tree_.roots()) co_yield gl;
}

template<typename ILogTrailAction>
coroutine<const goal_lineage*, void> dbuct_srt_active_goals<ILogTrailAction>::iterate_child_goals(const goal_lineage* gl) const {
    for (const goal_lineage* child : tree_.children(gl)) co_yield child;
}

#endif
