#ifndef DBUCT_SRT_ACTIVE_GOALS_HPP
#define DBUCT_SRT_ACTIVE_GOALS_HPP

#include <deque>
#include <list>
#include <set>
#include <stack>
#include "infrastructure/coroutine.hpp"
#include "infrastructure/dbuct_series_reduced_tree.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/srt_active_goals_action.hpp"
#include "debug_assert.hpp"

struct dbuct_srt_active_goals {
    dbuct_srt_active_goals();
    void insert_active_goal(const goal_lineage* gl);
    void link_srt_goal_batch_parent(const goal_lineage* parent);
    void flush_srt_goal_batch();
    const goal_lineage* get_parent_goal(const goal_lineage* gl) const;
    bool is_active_goal(const goal_lineage* gl) const;
    size_t active_goals_size() const;
    bool empty() const;
    coroutine<const goal_lineage*, void> iterate_root_goals() const;
    coroutine<const goal_lineage*, void> iterate_child_goals(const goal_lineage* gl) const;

    void push_frame();
    void pop_frame();

private:
    struct in_flight_frame {
        std::list<srt_active_goals_action> actions_;
    };

    using set_t = std::set<const goal_lineage*>;

    void log_in_flight(srt_active_goals_action action);
    void undo_in_flight_action(const srt_active_goals_action& action);

    dbuct_series_reduced_tree<const goal_lineage*> tree_;
    set_t in_flight_;
    std::stack<in_flight_frame> in_flight_frames_;
};

#endif
