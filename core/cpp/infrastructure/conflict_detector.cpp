#include "../../hpp/infrastructure/conflict_detector.hpp"

conflict_detector::conflict_detector(i_get_goal_candidate_rule_ids& get_goal_candidate_rule_ids)
    : get_goal_candidate_rule_ids(get_goal_candidate_rule_ids) {}

bool conflict_detector::detect(const goal_lineage* gl) const {
    return get_goal_candidate_rule_ids.get(gl).size() == 0;
}
