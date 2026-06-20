#include "infrastructure/goal_candidate_rules.hpp"

goal_candidate_rules::goal_candidate_rules(ra_rule_id_set_factory& factory)
    : factory_(factory) {}

ra_rule_id_set& goal_candidate_rules::get(const goal_lineage* gl) {
    return by_goal_.at(gl);
}

const ra_rule_id_set& goal_candidate_rules::get(const goal_lineage* gl) const {
    return by_goal_.at(gl);
}

void goal_candidate_rules::insert(const goal_lineage* gl) {
    DEBUG_ASSERT(!by_goal_.contains(gl));
    by_goal_.emplace(gl, factory_.make());
}

void goal_candidate_rules::link_goal_candidate(const goal_lineage* gl, rule_id r) {
    by_goal_.at(gl).insert(r);
}

void goal_candidate_rules::unlink_goal_candidate(const goal_lineage* gl, rule_id r) {
    by_goal_.at(gl).erase(r);
}

void goal_candidate_rules::erase(const goal_lineage* gl) {
    auto erased = by_goal_.erase(gl);
    DEBUG_ASSERT(erased == 1);
}

void goal_candidate_rules::clear_goal_candidate_rule_ids() {
    by_goal_.clear();
}
