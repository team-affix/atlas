#ifndef COMPUTE_FEWER_CANDIDATE_GOAL_VALUE_HPP
#define COMPUTE_FEWER_CANDIDATE_GOAL_VALUE_HPP

#include "value_objects/lineage.hpp"

template<typename IGetGoalCandidateRuleIds>
struct compute_fewer_candidate_goal_value {
    compute_fewer_candidate_goal_value(IGetGoalCandidateRuleIds&);
    double compute_active_goal_value(const goal_lineage* gl);
private:
    IGetGoalCandidateRuleIds& get_goal_candidate_rule_ids_;
};

template<typename IG>
compute_fewer_candidate_goal_value<IG>::compute_fewer_candidate_goal_value(IG& get_ids)
    : get_goal_candidate_rule_ids_(get_ids) {}

template<typename IG>
double compute_fewer_candidate_goal_value<IG>::compute_active_goal_value(
    const goal_lineage* gl) {
    return -static_cast<double>(get_goal_candidate_rule_ids_.get(gl).size());
}

#endif
