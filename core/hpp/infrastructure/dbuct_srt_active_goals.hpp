#ifndef DBUCT_SRT_ACTIVE_GOALS_HPP
#define DBUCT_SRT_ACTIVE_GOALS_HPP

#include <list>
#include <set>
#include <stack>
#include "infrastructure/coroutine.hpp"
#include "infrastructure/dbuct_series_reduced_tree.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/srt_active_goals_action.hpp"
#include "debug_assert.hpp"

struct dbuct_srt_active_goals {
    void insert_active_goal(const goal_lineage* gl);
    void link_srt_goal_batch_parent(const goal_lineage* parent);
    void flush_srt_goal_batch();
    bool is_active_goal(const goal_lineage* gl) const;
    size_t active_goals_size() const;
    bool empty() const;
    coroutine<const goal_lineage*, void> iterate_root_goals() const;
    coroutine<const goal_lineage*, void> iterate_child_goals(const goal_lineage* gl) const;

    void push_frame();
    void pop_frame();

private:
    struct in_flight_frame {
        std::list<srt_active_goals_action> actions;
    };

    using in_flight_t = std::set<const goal_lineage*>;

    void log_in_flight(srt_active_goals_action action);
    void undo_in_flight_action(const srt_active_goals_action& action);

    dbuct_series_reduced_tree<const goal_lineage*> tree_;
    in_flight_t in_flight_;
    std::stack<in_flight_frame> in_flight_frames_;
};

inline void dbuct_srt_active_goals::insert_active_goal(const goal_lineage* gl) {
    in_flight_.insert(gl);
    log_in_flight(srt_in_flight_insert{gl});
    const bool tree_inserted = tree_.insert(gl);
    DEBUG_ASSERT(tree_inserted);
}

inline void dbuct_srt_active_goals::link_srt_goal_batch_parent(const goal_lineage* parent) {
    tree_.link(parent, in_flight_);
}

inline void dbuct_srt_active_goals::flush_srt_goal_batch() {
    srt_in_flight_clear captured{std::move(in_flight_)};
    in_flight_.clear();
    log_in_flight(std::move(captured));
}

inline bool dbuct_srt_active_goals::is_active_goal(const goal_lineage* gl) const {
    return tree_.leaves().contains(gl);
}

inline size_t dbuct_srt_active_goals::active_goals_size() const {
    return tree_.leaves().size();
}

inline bool dbuct_srt_active_goals::empty() const {
    return tree_.leaves().empty();
}

inline coroutine<const goal_lineage*, void> dbuct_srt_active_goals::iterate_root_goals() const {
    for (const goal_lineage* gl : tree_.roots())
        co_yield gl;
}

inline coroutine<const goal_lineage*, void> dbuct_srt_active_goals::iterate_child_goals(
    const goal_lineage* gl) const {
    for (const goal_lineage* child : tree_.children(gl))
        co_yield child;
}

inline void dbuct_srt_active_goals::push_frame() {
    tree_.push_frame();
    in_flight_frames_.push(in_flight_frame{});
}

inline void dbuct_srt_active_goals::pop_frame() {
    tree_.pop_frame();
    auto current = std::move(in_flight_frames_.top());
    in_flight_frames_.pop();
    for (auto it = current.actions.rbegin(); it != current.actions.rend(); ++it)
        undo_in_flight_action(*it);
}

inline void dbuct_srt_active_goals::log_in_flight(srt_active_goals_action action) {
    DEBUG_ASSERT(!in_flight_frames_.empty());
    in_flight_frames_.top().actions.push_back(std::move(action));
}

inline void dbuct_srt_active_goals::undo_in_flight_action(const srt_active_goals_action& action) {
    if (const auto* ins = std::get_if<srt_in_flight_insert>(&action))
        in_flight_.erase(ins->gl);
    else {
        const auto& clr = std::get<srt_in_flight_clear>(action);
        in_flight_ = clr.saved;
    }
}

#endif
