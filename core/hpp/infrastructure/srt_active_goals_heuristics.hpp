#ifndef SRT_ACTIVE_GOALS_HEURISTICS_HPP
#define SRT_ACTIVE_GOALS_HEURISTICS_HPP

#include <algorithm>
#include <limits>
#include <unordered_map>
#include "value_objects/lineage.hpp"
#include "debug_assert.hpp"

template<typename IInsertActiveGoal, typename IClearActiveGoals,
         typename IGetParentGoal, typename IIterateChildGoals,
         typename ILinkSrtGoalBatchParent>
struct srt_active_goals_heuristics {
    srt_active_goals_heuristics(IInsertActiveGoal&, IClearActiveGoals&,
                                IGetParentGoal&, IIterateChildGoals&,
                                ILinkSrtGoalBatchParent&);

    void insert_active_goal(const goal_lineage* gl);
    void link_srt_goal_batch_parent(const goal_lineage* parent);
    void clear_active_goals();
    void set_active_goal_value(const goal_lineage* gl, double value);
    double get(const goal_lineage* gl) const;

private:
    using map_t = std::unordered_map<const goal_lineage*, double>;

    double max_child_score(const goal_lineage* parent) const;
    void percolate_from(const goal_lineage* parent);

    IInsertActiveGoal& insert_active_goal_;
    IClearActiveGoals& clear_active_goals_;
    IGetParentGoal& get_parent_goal_;
    IIterateChildGoals& iterate_child_goals_;
    ILinkSrtGoalBatchParent& link_srt_goal_batch_parent_;
    map_t scores_;
};

template<typename IIAG, typename ICAG, typename IPG, typename IICG, typename ILSP>
srt_active_goals_heuristics<IIAG, ICAG, IPG, IICG, ILSP>::srt_active_goals_heuristics(
    IIAG& insert_active_goal, ICAG& clear_active_goals,
    IPG& get_parent_goal, IICG& iterate_child_goals,
    ILSP& link_srt_goal_batch_parent)
    : insert_active_goal_(insert_active_goal)
    , clear_active_goals_(clear_active_goals)
    , get_parent_goal_(get_parent_goal)
    , iterate_child_goals_(iterate_child_goals)
    , link_srt_goal_batch_parent_(link_srt_goal_batch_parent) {}

template<typename IIAG, typename ICAG, typename IPG, typename IICG, typename ILSP>
void srt_active_goals_heuristics<IIAG, ICAG, IPG, IICG, ILSP>::insert_active_goal(
    const goal_lineage* gl) {
    insert_active_goal_.insert_active_goal(gl);
    auto [_, inserted] = scores_.emplace(gl, 0.0);
    DEBUG_ASSERT(inserted);
}

template<typename IIAG, typename ICAG, typename IPG, typename IICG, typename ILSP>
void srt_active_goals_heuristics<IIAG, ICAG, IPG, IICG, ILSP>::link_srt_goal_batch_parent(
    const goal_lineage* parent) {
    scores_.at(parent) = -std::numeric_limits<double>::infinity();
    percolate_from(get_parent_goal_.get_parent_goal(parent));
    link_srt_goal_batch_parent_.link_srt_goal_batch_parent(parent);
}

template<typename IIAG, typename ICAG, typename IPG, typename IICG, typename ILSP>
void srt_active_goals_heuristics<IIAG, ICAG, IPG, IICG, ILSP>::clear_active_goals() {
    clear_active_goals_.clear_active_goals();
    scores_.clear();
}

template<typename IIAG, typename ICAG, typename IPG, typename IICG, typename ILSP>
void srt_active_goals_heuristics<IIAG, ICAG, IPG, IICG, ILSP>::set_active_goal_value(
    const goal_lineage* gl, double value) {
    scores_.at(gl) = value;
    percolate_from(get_parent_goal_.get_parent_goal(gl));
}

template<typename IIAG, typename ICAG, typename IPG, typename IICG, typename ILSP>
double srt_active_goals_heuristics<IIAG, ICAG, IPG, IICG, ILSP>::get(
    const goal_lineage* gl) const {
    return scores_.at(gl);
}

template<typename IIAG, typename ICAG, typename IPG, typename IICG, typename ILSP>
double srt_active_goals_heuristics<IIAG, ICAG, IPG, IICG, ILSP>::max_child_score(
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

template<typename IIAG, typename ICAG, typename IPG, typename IICG, typename ILSP>
void srt_active_goals_heuristics<IIAG, ICAG, IPG, IICG, ILSP>::percolate_from(
    const goal_lineage* parent) {
    while (parent != nullptr) {
        const double new_val = max_child_score(parent);
        if (new_val == scores_.at(parent)) return;
        scores_.at(parent) = new_val;
        parent = get_parent_goal_.get_parent_goal(parent);
    }
}

#endif
