#ifndef UNIT_GOAL_DETECTOR_HPP
#define UNIT_GOAL_DETECTOR_HPP

#include "infrastructure/locator.hpp"
#include "interfaces/i_detect_unit_goal.hpp"
#include "interfaces/i_get_goal_candidate_rule_ids.hpp"

struct unit_goal_detector : i_detect_unit_goal {
    unit_goal_detector(locator& loc);
    bool detect(const goal_lineage*) const override;
private:
    i_get_goal_candidate_rule_ids& get_goal_candidate_rule_ids;
};

#endif
