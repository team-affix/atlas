#ifndef CONFLICT_DETECTOR_HPP
#define CONFLICT_DETECTOR_HPP

#include "interfaces/i_conflict_detector.hpp"
#include "interfaces/i_get_goal_candidate_rule_ids.hpp"

struct conflict_detector : i_conflict_detector {
    conflict_detector(i_get_goal_candidate_rule_ids& get_goal_candidate_rule_ids);
    bool detect(const goal_lineage*) const override;
private:
    i_get_goal_candidate_rule_ids& get_goal_candidate_rule_ids;
};

#endif
