#include "../../hpp/infrastructure/goal_candidate_rules.hpp"

i_rule_set& goal_candidate_rules::get(const goal_lineage* gl) {
    return by_goal_[gl];
}

const i_rule_set& goal_candidate_rules::get(const goal_lineage* gl) const {
    auto it = by_goal_.find(gl);
    if (it == by_goal_.end())
        return empty_;
    return it->second;
}

void goal_candidate_rules::link_goal_candidate(const goal_lineage* gl, const rule* r) {
    by_goal_[gl].insert(r);
}

void goal_candidate_rules::unlink_goal_candidate(const goal_lineage* gl, const rule* r) {
    auto it = by_goal_.find(gl);
    if (it == by_goal_.end())
        return;
    it->second.erase(r);
}

void goal_candidate_rules::constrain_goal_candidate_rules(const resolution_lineage* rl) {
    by_goal_.erase(rl->parent);
}
