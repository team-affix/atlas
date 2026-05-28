#include "../../hpp/infrastructure/unit_goal_detector.hpp"

unit_goal_detector::unit_goal_detector(i_get_goal_candidate_rule_ids& get_goal_candidate_rule_ids)
    : get_goal_candidate_rule_ids(get_goal_candidate_rule_ids) {}

bool unit_goal_detector::detect(const goal_lineage* gl) const {
    return get_goal_candidate_rule_ids.get(gl).size() == 1;
}
