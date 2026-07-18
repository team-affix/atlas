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
    const goal_lineage* get_parent_goal(const goal_lineage*) const;
    coroutine<const goal_lineage*, void> iterate_root_goals() const;
    coroutine<const goal_lineage*, void> iterate_child_goals(const goal_lineage*) const;
    coroutine<const goal_lineage*, void> iterate_srt_goal_batch() const;
private:
    series_reduced_tree<const goal_lineage*> tree_;
    std::set<const goal_lineage*> in_flight_;
};

#endif
