#include "infrastructure/conflict_detector.hpp"

conflict_detector::conflict_detector(locator& loc)
    : get_goal_candidate_rule_ids(loc.locate<i_get_goal_candidate_rule_ids>()) {}

bool conflict_detector::detect(const goal_lineage* gl) const {
    return get_goal_candidate_rule_ids.get(gl).size() == 0;
}
