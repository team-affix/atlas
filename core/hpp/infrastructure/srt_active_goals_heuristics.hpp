#ifndef SRT_ACTIVE_GOALS_HEURISTICS_HPP
#define SRT_ACTIVE_GOALS_HEURISTICS_HPP

#include <algorithm>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include "value_objects/lineage.hpp"
#include "debug_assert.hpp"

template<typename IGetParentGoal, typename IIterateChildGoals, typename IComputeActiveGoalValue>
struct srt_active_goals_heuristics {
    srt_active_goals_heuristics(IGetParentGoal&, IIterateChildGoals&, IComputeActiveGoalValue&);

    void insert_active_goal(const goal_lineage* gl);
    void flush_srt_goal_batch();
    void clear_active_goals();
    double get(const goal_lineage* gl) const;

private:
    using map_t = std::unordered_map<const goal_lineage*, double>;
    using set_t = std::unordered_set<const goal_lineage*>;

    double max_child_score(const goal_lineage* parent) const;
    void percolate_from(const goal_lineage* parent);

    IGetParentGoal& get_parent_goal_;
    IIterateChildGoals& iterate_child_goals_;
    IComputeActiveGoalValue& compute_active_goal_value_;
    map_t scores_;
    set_t pending_;
};

template<typename IPG, typename IICG, typename ICAV>
srt_active_goals_heuristics<IPG, IICG, ICAV>::srt_active_goals_heuristics(
    IPG& get_parent_goal, IICG& iterate_child_goals, ICAV& compute_active_goal_value)
    : get_parent_goal_(get_parent_goal)
    , iterate_child_goals_(iterate_child_goals)
    , compute_active_goal_value_(compute_active_goal_value) {}

template<typename IPG, typename IICG, typename ICAV>
void srt_active_goals_heuristics<IPG, IICG, ICAV>::insert_active_goal(const goal_lineage* gl) {
    auto [_, inserted] = scores_.emplace(
        gl, compute_active_goal_value_.compute_active_goal_value(gl));
    DEBUG_ASSERT(inserted);
    auto [__, pending_inserted] = pending_.insert(gl);
    DEBUG_ASSERT(pending_inserted);
}

template<typename IPG, typename IICG, typename ICAV>
void srt_active_goals_heuristics<IPG, IICG, ICAV>::flush_srt_goal_batch() {
    if (pending_.empty()) return;
    percolate_from(get_parent_goal_.get_parent_goal(*pending_.begin()));
    pending_.clear();
}

template<typename IPG, typename IICG, typename ICAV>
void srt_active_goals_heuristics<IPG, IICG, ICAV>::clear_active_goals() {
    scores_.clear();
    pending_.clear();
}

template<typename IPG, typename IICG, typename ICAV>
double srt_active_goals_heuristics<IPG, IICG, ICAV>::get(const goal_lineage* gl) const {
    return scores_.at(gl);
}

template<typename IPG, typename IICG, typename ICAV>
double srt_active_goals_heuristics<IPG, IICG, ICAV>::max_child_score(
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
void srt_active_goals_heuristics<IPG, IICG, ICAV>::percolate_from(const goal_lineage* parent) {
    while (parent != nullptr) {
        scores_.at(parent) = max_child_score(parent);
        parent = get_parent_goal_.get_parent_goal(parent);
    }
}

#endif
