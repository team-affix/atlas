#ifndef SRT_ACTIVE_GOALS_HPP
#define SRT_ACTIVE_GOALS_HPP

#include <set>
#include "infrastructure/series_reduced_tree.hpp"
#include "infrastructure/coroutine.hpp"
#include "value_objects/lineage.hpp"
#include "debug_assert.hpp"

struct srt_active_goals {
    void insert_active_goal(const goal_lineage*);
    bool is_active_goal(const goal_lineage*) const;
    size_t active_goals_size() const;
    bool empty() const;
    void clear_active_goals();
    void link_srt_goal_batch_parent(const goal_lineage*);
    void flush_srt_goal_batch();
    coroutine<const goal_lineage*, void> iterate_root_goals() const;
    coroutine<const goal_lineage*, void> iterate_child_goals(const goal_lineage*) const;
private:
    series_reduced_tree<const goal_lineage*> tree_;
    std::set<const goal_lineage*> in_flight_;
};

inline void srt_active_goals::insert_active_goal(const goal_lineage* gl) {
    auto [_, inserted] = in_flight_.emplace(gl);
    DEBUG_ASSERT(inserted);
    const bool tree_inserted = tree_.insert(gl);
    DEBUG_ASSERT(tree_inserted);
}

inline void srt_active_goals::link_srt_goal_batch_parent(const goal_lineage* parent) {
    tree_.link(parent, in_flight_);
}

inline void srt_active_goals::flush_srt_goal_batch() {
    in_flight_.clear();
}

inline bool srt_active_goals::is_active_goal(const goal_lineage* gl) const {
    return tree_.leaves().contains(gl);
}

inline size_t srt_active_goals::active_goals_size() const {
    return tree_.leaves().size();
}

inline bool srt_active_goals::empty() const {
    return tree_.leaves().empty();
}

inline void srt_active_goals::clear_active_goals() {
    tree_.clear();
    in_flight_.clear();
}

inline coroutine<const goal_lineage*, void> srt_active_goals::iterate_root_goals() const {
    for (const goal_lineage* gl : tree_.roots()) co_yield gl;
}

inline coroutine<const goal_lineage*, void> srt_active_goals::iterate_child_goals(const goal_lineage* gl) const {
    for (const goal_lineage* child : tree_.children(gl)) co_yield child;
}

#endif
