#ifndef CONFLICT_DETECTOR_HPP
#define CONFLICT_DETECTOR_HPP

#include "../interfaces/i_conflict_detector.hpp"
#include "../interfaces/i_get_goal_candidate_rules.hpp"

struct conflict_detector : i_conflict_detector {
    conflict_detector(i_get_goal_candidate_rules& ggcr);
    bool detect(const goal_lineage*) override;
private:
    i_get_goal_candidate_rules& ggcr;
};

#endif
