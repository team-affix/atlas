#ifndef UNIT_GOAL_DETECTOR_HPP
#define UNIT_GOAL_DETECTOR_HPP

#include "../interfaces/i_unit_goal_detector.hpp"
#include "../interfaces/i_get_goal_candidate_rules.hpp"

struct unit_goal_detector : i_unit_goal_detector {
    unit_goal_detector(i_get_goal_candidate_rules& ggcr);
    bool detect(const goal_lineage*) const override;
private:
    i_get_goal_candidate_rules& ggcr;
};

#endif
