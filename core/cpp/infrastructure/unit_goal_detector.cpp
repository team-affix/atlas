#include "../../hpp/infrastructure/unit_goal_detector.hpp"

unit_goal_detector::unit_goal_detector(i_get_goal_candidates_size& ggcs) : ggcs(ggcs) {}

bool unit_goal_detector::detect(const goal_lineage* gl) const {
    return ggcs.size(gl) == 1;
}
