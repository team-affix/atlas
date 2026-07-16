#ifndef CONFLICT_DETECTOR_HPP
#define CONFLICT_DETECTOR_HPP

#include "value_objects/lineage.hpp"

template<typename IGetGoalCandidateRuleIds>
struct conflict_detector {
    conflict_detector(IGetGoalCandidateRuleIds& gcr);
    bool detect(const goal_lineage*) const;
private:
    IGetGoalCandidateRuleIds& get_goal_candidate_rule_ids_;
};

template<typename IGetGoalCandidateRuleIds>
conflict_detector<IGetGoalCandidateRuleIds>::conflict_detector(IGetGoalCandidateRuleIds& gcr)
    : get_goal_candidate_rule_ids_(gcr) {}

template<typename IGetGoalCandidateRuleIds>
bool conflict_detector<IGetGoalCandidateRuleIds>::detect(const goal_lineage* gl) const {
    return get_goal_candidate_rule_ids_.get(gl).size() == 0;
}

#endif
