#ifndef DBUCT_SRT_ACTIVE_GOALS_HEURISTICS_HPP
#define DBUCT_SRT_ACTIVE_GOALS_HEURISTICS_HPP

#include <algorithm>
#include <deque>
#include <limits>
#include <list>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include "value_objects/lineage.hpp"
#include "value_objects/srt_active_goals_heuristics_action.hpp"
#include "debug_assert.hpp"

template<typename IGetParentGoal, typename IIterateChildGoals, typename IComputeActiveGoalValue>
struct dbuct_srt_active_goals_heuristics {
    dbuct_srt_active_goals_heuristics(IGetParentGoal&, IIterateChildGoals&, IComputeActiveGoalValue&);

    void insert_active_goal(const goal_lineage* gl);
    void flush_srt_goal_batch();
    void clear_active_goals();
    double get(const goal_lineage* gl) const;

    void push_frame();
    void pop_frame();

private:
    using map_t = std::unordered_map<const goal_lineage*, double>;
    using set_t = std::unordered_set<const goal_lineage*>;

    struct frame {
        std::list<srt_active_goals_heuristics_action> actions_;
    };

    double max_child_score(const goal_lineage* parent) const;
    void percolate_from(const goal_lineage* parent);
    void assign_score(const goal_lineage* gl, double value);
    void log(srt_active_goals_heuristics_action action);
    void undo_action(const srt_active_goals_heuristics_action& action);

    IGetParentGoal& get_parent_goal_;
    IIterateChildGoals& iterate_child_goals_;
    IComputeActiveGoalValue& compute_active_goal_value_;
    map_t scores_;
    set_t pending_;
    std::stack<frame> frame_stack_;
};

template<typename IPG, typename IICG, typename ICAV>
dbuct_srt_active_goals_heuristics<IPG, IICG, ICAV>::dbuct_srt_active_goals_heuristics(
    IPG& get_parent_goal, IICG& iterate_child_goals, ICAV& compute_active_goal_value)
    : get_parent_goal_(get_parent_goal)
    , iterate_child_goals_(iterate_child_goals)
    , compute_active_goal_value_(compute_active_goal_value)
    , frame_stack_(std::deque<frame>{frame{}}) {}

template<typename IPG, typename IICG, typename ICAV>
void dbuct_srt_active_goals_heuristics<IPG, IICG, ICAV>::insert_active_goal(
    const goal_lineage* gl) {
    const double value = compute_active_goal_value_.compute_active_goal_value(gl);
    auto [_, inserted] = scores_.emplace(gl, value);
    DEBUG_ASSERT(inserted);
    log(srt_heuristics_score_insert{gl});
    auto [__, pending_inserted] = pending_.insert(gl);
    DEBUG_ASSERT(pending_inserted);
    log(srt_heuristics_pending_insert{gl});
}

template<typename IPG, typename IICG, typename ICAV>
void dbuct_srt_active_goals_heuristics<IPG, IICG, ICAV>::flush_srt_goal_batch() {
    if (pending_.empty()) return;
    percolate_from(get_parent_goal_.get_parent_goal(*pending_.begin()));
    srt_heuristics_pending_clear captured{std::move(pending_)};
    pending_.clear();
    log(std::move(captured));
}

template<typename IPG, typename IICG, typename ICAV>
void dbuct_srt_active_goals_heuristics<IPG, IICG, ICAV>::clear_active_goals() {
    srt_heuristics_scores_clear scores_captured{std::move(scores_)};
    scores_.clear();
    log(std::move(scores_captured));
    srt_heuristics_pending_clear pending_captured{std::move(pending_)};
    pending_.clear();
    log(std::move(pending_captured));
}

template<typename IPG, typename IICG, typename ICAV>
double dbuct_srt_active_goals_heuristics<IPG, IICG, ICAV>::get(const goal_lineage* gl) const {
    return scores_.at(gl);
}

template<typename IPG, typename IICG, typename ICAV>
void dbuct_srt_active_goals_heuristics<IPG, IICG, ICAV>::push_frame() {
    frame_stack_.push(frame{});
}

template<typename IPG, typename IICG, typename ICAV>
void dbuct_srt_active_goals_heuristics<IPG, IICG, ICAV>::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions_.rbegin(); it != current.actions_.rend(); ++it)
        undo_action(*it);
}

template<typename IPG, typename IICG, typename ICAV>
double dbuct_srt_active_goals_heuristics<IPG, IICG, ICAV>::max_child_score(
    const goal_lineage* parent) const {
    double best = -std::numeric_limits<double>::infinity();
    auto sm = iterate_child_goals_.iterate_child_goals(parent);
    while (!sm.done()) {
        sm.resume();
        if (sm.has_yield())
            best = std::max(best, scores_.at(sm.consume_yield()));
    }
    return best;
}

template<typename IPG, typename IICG, typename ICAV>
void dbuct_srt_active_goals_heuristics<IPG, IICG, ICAV>::percolate_from(
    const goal_lineage* parent) {
    while (parent != nullptr) {
        assign_score(parent, max_child_score(parent));
        parent = get_parent_goal_.get_parent_goal(parent);
    }
}

template<typename IPG, typename IICG, typename ICAV>
void dbuct_srt_active_goals_heuristics<IPG, IICG, ICAV>::assign_score(
    const goal_lineage* gl, double value) {
    double& slot = scores_.at(gl);
    const double previous = slot;
    slot = value;
    log(srt_heuristics_score_assign{gl, previous});
}

template<typename IPG, typename IICG, typename ICAV>
void dbuct_srt_active_goals_heuristics<IPG, IICG, ICAV>::log(
    srt_active_goals_heuristics_action action) {
    DEBUG_ASSERT(!frame_stack_.empty());
    frame_stack_.top().actions_.push_back(std::move(action));
}

template<typename IPG, typename IICG, typename ICAV>
void dbuct_srt_active_goals_heuristics<IPG, IICG, ICAV>::undo_action(
    const srt_active_goals_heuristics_action& action) {
    if (const auto* insert = std::get_if<srt_heuristics_score_insert>(&action)) {
        scores_.erase(insert->gl);
    } else if (const auto* assign = std::get_if<srt_heuristics_score_assign>(&action)) {
        scores_.at(assign->gl) = assign->previous;
    } else if (const auto* scores_clear = std::get_if<srt_heuristics_scores_clear>(&action)) {
        scores_ = scores_clear->previous;
    } else if (const auto* pending_insert = std::get_if<srt_heuristics_pending_insert>(&action)) {
        pending_.erase(pending_insert->gl);
    } else {
        const auto& pending_clear = std::get<srt_heuristics_pending_clear>(action);
        pending_ = pending_clear.previous;
    }
}

#endif
