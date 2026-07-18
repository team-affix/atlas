#ifndef DBUCT_RP_SRT_ACTIVE_GOALS_HPP
#define DBUCT_RP_SRT_ACTIVE_GOALS_HPP

#include <algorithm>
#include <deque>
#include <limits>
#include <list>
#include <stack>
#include <unordered_map>
#include "value_objects/lineage.hpp"
#include "value_objects/rp_srt_active_goals_action.hpp"
#include "debug_assert.hpp"

template<typename IInsertActiveGoal, typename IGetParentGoal, typename IIterateChildGoals>
struct dbuct_rp_srt_active_goals {
    dbuct_rp_srt_active_goals(IInsertActiveGoal&, IGetParentGoal&, IIterateChildGoals&);

    void insert_active_goal(const goal_lineage* gl);
    void set_active_goal_value(const goal_lineage* gl, double value);
    double get(const goal_lineage* gl) const;

    void push_frame();
    void pop_frame();

private:
    using map_t = std::unordered_map<const goal_lineage*, double>;

    struct frame {
        std::list<rp_srt_active_goals_action> actions_;
    };

    double max_child_score(const goal_lineage* parent) const;
    void percolate_from(const goal_lineage* parent);
    void assign_score(const goal_lineage* gl, double value);
    void log(rp_srt_active_goals_action action);
    void undo_action(const rp_srt_active_goals_action& action);

    IInsertActiveGoal& insert_active_goal_;
    IGetParentGoal& get_parent_goal_;
    IIterateChildGoals& iterate_child_goals_;
    map_t scores_;
    std::stack<frame> frame_stack_;
};

template<typename IIAG, typename IPG, typename IICG>
dbuct_rp_srt_active_goals<IIAG, IPG, IICG>::dbuct_rp_srt_active_goals(
    IIAG& insert_active_goal, IPG& get_parent_goal, IICG& iterate_child_goals)
    : insert_active_goal_(insert_active_goal)
    , get_parent_goal_(get_parent_goal)
    , iterate_child_goals_(iterate_child_goals)
    , frame_stack_(std::deque<frame>{frame{}}) {}

template<typename IIAG, typename IPG, typename IICG>
void dbuct_rp_srt_active_goals<IIAG, IPG, IICG>::insert_active_goal(
    const goal_lineage* gl) {
    insert_active_goal_.insert_active_goal(gl);
    auto [_, inserted] = scores_.emplace(gl, 0.0);
    DEBUG_ASSERT(inserted);
    log(rp_srt_score_insert{gl});
}

template<typename IIAG, typename IPG, typename IICG>
void dbuct_rp_srt_active_goals<IIAG, IPG, IICG>::set_active_goal_value(
    const goal_lineage* gl, double value) {
    assign_score(gl, value);
    percolate_from(get_parent_goal_.get_parent_goal(gl));
}

template<typename IIAG, typename IPG, typename IICG>
double dbuct_rp_srt_active_goals<IIAG, IPG, IICG>::get(
    const goal_lineage* gl) const {
    return scores_.at(gl);
}

template<typename IIAG, typename IPG, typename IICG>
void dbuct_rp_srt_active_goals<IIAG, IPG, IICG>::push_frame() {
    frame_stack_.push(frame{});
}

template<typename IIAG, typename IPG, typename IICG>
void dbuct_rp_srt_active_goals<IIAG, IPG, IICG>::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions_.rbegin(); it != current.actions_.rend(); ++it)
        undo_action(*it);
}

template<typename IIAG, typename IPG, typename IICG>
double dbuct_rp_srt_active_goals<IIAG, IPG, IICG>::max_child_score(
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

template<typename IIAG, typename IPG, typename IICG>
void dbuct_rp_srt_active_goals<IIAG, IPG, IICG>::percolate_from(
    const goal_lineage* parent) {
    while (parent != nullptr) {
        const double new_val = max_child_score(parent);
        if (new_val == scores_.at(parent)) return;
        assign_score(parent, new_val);
        parent = get_parent_goal_.get_parent_goal(parent);
    }
}

template<typename IIAG, typename IPG, typename IICG>
void dbuct_rp_srt_active_goals<IIAG, IPG, IICG>::assign_score(
    const goal_lineage* gl, double value) {
    double& slot = scores_.at(gl);
    const double previous = slot;
    slot = value;
    log(rp_srt_score_assign{gl, previous});
}

template<typename IIAG, typename IPG, typename IICG>
void dbuct_rp_srt_active_goals<IIAG, IPG, IICG>::log(
    rp_srt_active_goals_action action) {
    DEBUG_ASSERT(!frame_stack_.empty());
    frame_stack_.top().actions_.push_back(std::move(action));
}

template<typename IIAG, typename IPG, typename IICG>
void dbuct_rp_srt_active_goals<IIAG, IPG, IICG>::undo_action(
    const rp_srt_active_goals_action& action) {
    if (const auto* insert = std::get_if<rp_srt_score_insert>(&action)) {
        scores_.erase(insert->gl);
    } else {
        const auto& assign = std::get<rp_srt_score_assign>(action);
        scores_.at(assign.gl) = assign.previous;
    }
}

#endif
