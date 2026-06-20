#ifndef GOAL_CANDIDATE_RULES_HPP
#define GOAL_CANDIDATE_RULES_HPP

#include <unordered_map>
#include "infrastructure/ra_rule_id_set_factory.hpp"
#include "infrastructure/ra_rule_id_set.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"
#include "debug_assert.hpp"

struct goal_candidate_rules {
    explicit goal_candidate_rules(ra_rule_id_set_factory&);
    ra_rule_id_set& get(const goal_lineage*);
    const ra_rule_id_set& get(const goal_lineage*) const;
    void insert(const goal_lineage*);
    void link_goal_candidate(const goal_lineage*, rule_id);
    void unlink_goal_candidate(const goal_lineage*, rule_id);
    void erase(const goal_lineage*);
    void clear_goal_candidate_rule_ids();
private:
    ra_rule_id_set_factory& factory_;
    std::unordered_map<const goal_lineage*, ra_rule_id_set> by_goal_;
};

inline goal_candidate_rules::goal_candidate_rules(ra_rule_id_set_factory& factory)
    : factory_(factory) {}

inline ra_rule_id_set& goal_candidate_rules::get(const goal_lineage* gl) {
    return by_goal_.at(gl);
}

inline const ra_rule_id_set& goal_candidate_rules::get(const goal_lineage* gl) const {
    return by_goal_.at(gl);
}

inline void goal_candidate_rules::insert(const goal_lineage* gl) {
    DEBUG_ASSERT(!by_goal_.contains(gl));
    by_goal_.emplace(gl, factory_.make());
}

inline void goal_candidate_rules::link_goal_candidate(const goal_lineage* gl, rule_id r) {
    by_goal_.at(gl).insert(r);
}

inline void goal_candidate_rules::unlink_goal_candidate(const goal_lineage* gl, rule_id r) {
    by_goal_.at(gl).erase(r);
}

inline void goal_candidate_rules::erase(const goal_lineage* gl) {
    auto erased = by_goal_.erase(gl);
    DEBUG_ASSERT(erased == 1);
}

inline void goal_candidate_rules::clear_goal_candidate_rule_ids() {
    by_goal_.clear();
}

#endif
