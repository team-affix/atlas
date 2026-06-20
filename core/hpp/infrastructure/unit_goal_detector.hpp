#ifndef UNIT_GOAL_DETECTOR_HPP
#define UNIT_GOAL_DETECTOR_HPP

#include "value_objects/lineage.hpp"

template<typename IGetGoalCandidateRuleIds>
struct unit_goal_detector {
    unit_goal_detector(IGetGoalCandidateRuleIds& gcr);
    bool detect(const goal_lineage*) const;
private:
    IGetGoalCandidateRuleIds& get_goal_candidate_rule_ids;
};

template<typename IGetGoalCandidateRuleIds>
unit_goal_detector<IGetGoalCandidateRuleIds>::unit_goal_detector(IGetGoalCandidateRuleIds& gcr)
    : get_goal_candidate_rule_ids(gcr) {}

template<typename IGetGoalCandidateRuleIds>
bool unit_goal_detector<IGetGoalCandidateRuleIds>::detect(const goal_lineage* gl) const {
    return get_goal_candidate_rule_ids.get(gl).size() == 1;
}

#endif
