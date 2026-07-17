#ifndef FEWER_CANDIDATE_GOAL_ROLLOUT_HPP
#define FEWER_CANDIDATE_GOAL_ROLLOUT_HPP

#include <limits>
#include <vector>
#include "value_objects/lineage.hpp"

template<typename IGetGoalHeuristicScore>
struct fewer_candidate_goal_rollout {
    fewer_candidate_goal_rollout(IGetGoalHeuristicScore&);
    const goal_lineage* rollout_choose_goal(const std::vector<const goal_lineage*>& goals);
private:
    IGetGoalHeuristicScore& get_goal_heuristic_score_;
};

template<typename IG>
fewer_candidate_goal_rollout<IG>::fewer_candidate_goal_rollout(IG& get_score)
    : get_goal_heuristic_score_(get_score) {}

template<typename IG>
const goal_lineage* fewer_candidate_goal_rollout<IG>::rollout_choose_goal(
    const std::vector<const goal_lineage*>& goals) {
    size_t best_i = 0;
    double best_score = -std::numeric_limits<double>::infinity();
    for (size_t i = 0; i < goals.size(); ++i) {
        const double score = get_goal_heuristic_score_.get(goals[i]);
        if (score > best_score) {
            best_score = score;
            best_i = i;
        }
    }
    return goals[best_i];
}

#endif
