#include "infrastructure/goal_candidate_rules.hpp"

i_rule_id_set& goal_candidate_rules::get(const goal_lineage* gl) {
    return by_goal_[gl];
}

const i_rule_id_set& goal_candidate_rules::get(const goal_lineage* gl) const {
    auto it = by_goal_.find(gl);
    if (it == by_goal_.end())
        return empty_;
    return it->second;
}

void goal_candidate_rules::link_goal_candidate(const goal_lineage* gl, rule_id r) {
    by_goal_[gl].insert(r);
}

void goal_candidate_rules::unlink_goal_candidate(const goal_lineage* gl, rule_id r) {
    auto it = by_goal_.find(gl);
    if (it == by_goal_.end())
        return;
    it->second.erase(r);
}

void goal_candidate_rules::erase(const goal_lineage* gl) {
    by_goal_.erase(gl);
}

void goal_candidate_rules::clear_goal_candidate_rule_ids() {
    by_goal_.clear();
}
