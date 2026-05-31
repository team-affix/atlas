#include "infrastructure/goal_candidate_rules.hpp"
#include "debug_assert.hpp"

i_rule_id_set& goal_candidate_rules::get(const goal_lineage* gl) {
    return by_goal_.at(gl);
}

const i_rule_id_set& goal_candidate_rules::get(const goal_lineage* gl) const {
    return by_goal_.at(gl);
}

void goal_candidate_rules::insert(const goal_lineage* gl) {
    auto [_, inserted] = by_goal_.emplace(gl, rule_id_set{});
    DEBUG_ASSERT(inserted);
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
