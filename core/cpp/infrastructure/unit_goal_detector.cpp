#include "../../hpp/infrastructure/unit_goal_detector.hpp"

unit_goal_detector::unit_goal_detector(i_get_goal_candidate_rules& ggcr) : ggcr(ggcr) {}

bool unit_goal_detector::detect(const goal_lineage* gl) const {
    const auto& candidate_rules = ggcr.get(gl);
    return candidate_rules.size() == 1;
}
