#ifndef SRT_ACTIVE_GOALS_HPP
#define SRT_ACTIVE_GOALS_HPP

#include <set>
#include "infrastructure/series_reduced_tree.hpp"
#include "interfaces/i_insert_active_goal.hpp"
#include "interfaces/i_is_active_goal.hpp"
#include "interfaces/i_iterate_child_goals.hpp"
#include "interfaces/i_iterate_root_goals.hpp"
#include "interfaces/i_active_goals_size.hpp"
#include "interfaces/i_check_active_goals_empty.hpp"
#include "interfaces/i_clear_active_goals.hpp"
#include "interfaces/i_srt_link_goal_batch_parent.hpp"
#include "interfaces/i_srt_flush_goal_batch.hpp"

struct srt_active_goals
    : i_insert_active_goal
    , i_is_active_goal
    , i_iterate_root_goals
    , i_iterate_child_goals
    , i_active_goals_size
    , i_check_active_goals_empty
    , i_clear_active_goals
    , i_srt_link_goal_batch_parent
    , i_srt_flush_goal_batch {
    void insert_active_goal(const goal_lineage*) override;
    bool is_active_goal(const goal_lineage*) const override;
    size_t active_goals_size() const override;
    bool empty() const override;
    void clear_active_goals() override;
    void link_srt_goal_batch_parent(const goal_lineage*) override;
    void flush_srt_goal_batch() override;
    coroutine<const goal_lineage*, void> iterate_root_goals() const override;
    coroutine<const goal_lineage*, void> iterate_child_goals(const goal_lineage*) const override;
private:
    series_reduced_tree<const goal_lineage*> tree_;
    std::set<const goal_lineage*> in_flight_;
};

#endif
