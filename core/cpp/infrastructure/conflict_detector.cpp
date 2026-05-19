#include "../../hpp/infrastructure/conflict_detector.hpp"

conflict_detector::conflict_detector(i_get_goal_candidate_rules& ggcr) : ggcr(ggcr) {}

bool conflict_detector::detect(const goal_lineage* goal) {
    auto& candidates = ggcr.get(goal);
    return candidates.size() == 0;
}
