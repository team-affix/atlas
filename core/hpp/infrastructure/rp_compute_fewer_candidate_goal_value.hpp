#ifndef RP_COMPUTE_FEWER_CANDIDATE_GOAL_VALUE_HPP
#define RP_COMPUTE_FEWER_CANDIDATE_GOAL_VALUE_HPP

#include "value_objects/lineage.hpp"

template<typename IGetGoalCandidateRuleIds>
struct rp_compute_fewer_candidate_goal_value {
    rp_compute_fewer_candidate_goal_value(IGetGoalCandidateRuleIds&);
    double compute_active_goal_value(const goal_lineage* gl);
private:
    IGetGoalCandidateRuleIds& get_goal_candidate_rule_ids_;
};

template<typename IG>
rp_compute_fewer_candidate_goal_value<IG>::rp_compute_fewer_candidate_goal_value(IG& get_ids)
    : get_goal_candidate_rule_ids_(get_ids) {}

template<typename IG>
double rp_compute_fewer_candidate_goal_value<IG>::compute_active_goal_value(
    const goal_lineage* gl) {
    return -static_cast<double>(get_goal_candidate_rule_ids_.get(gl).size());
}

#endif
