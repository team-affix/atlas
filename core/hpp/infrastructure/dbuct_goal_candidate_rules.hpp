#ifndef DBUCT_GOAL_CANDIDATE_RULES_HPP
#define DBUCT_GOAL_CANDIDATE_RULES_HPP

#include <unordered_map>
#include "infrastructure/ra_rule_id_set_factory.hpp"
#include "infrastructure/ra_rule_id_set.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"
#include "debug_assert.hpp"

// Delayed-backtracking variant of goal_candidate_rules.
//
// The active candidate set per goal is the structure that CDCL re-application
// prunes after a backtrack. Under DBUCT it is carried across episodes and
// restored to any choice boundary. ra_rule_id_set is copyable, so a snapshot is
// just a copy of the by-goal map (membership is restored exactly; note the
// random-access iteration order within a set may differ after erase+restore,
// which only affects exploration order, not membership/correctness).
struct dbuct_goal_candidate_rules {
    using snapshot_t = std::unordered_map<const goal_lineage*, ra_rule_id_set>;

    explicit dbuct_goal_candidate_rules(ra_rule_id_set_factory& factory);

    ra_rule_id_set& get(const goal_lineage* gl);
    const ra_rule_id_set& get(const goal_lineage* gl) const;
    void insert(const goal_lineage* gl);
    void link_goal_candidate(const goal_lineage* gl, rule_id r);
    void unlink_goal_candidate(const goal_lineage* gl, rule_id r);
    void erase(const goal_lineage* gl);
    void clear_goal_candidate_rule_ids();

    snapshot_t snapshot() const;
    void restore(snapshot_t s);

private:
    ra_rule_id_set_factory& factory_;
    std::unordered_map<const goal_lineage*, ra_rule_id_set> by_goal_;
};

inline dbuct_goal_candidate_rules::dbuct_goal_candidate_rules(ra_rule_id_set_factory& factory)
    : factory_(factory) {}

inline ra_rule_id_set& dbuct_goal_candidate_rules::get(const goal_lineage* gl) { return by_goal_.at(gl); }
inline const ra_rule_id_set& dbuct_goal_candidate_rules::get(const goal_lineage* gl) const { return by_goal_.at(gl); }

inline void dbuct_goal_candidate_rules::insert(const goal_lineage* gl) {
    DEBUG_ASSERT(!by_goal_.contains(gl));
    by_goal_.emplace(gl, factory_.make());
}

inline void dbuct_goal_candidate_rules::link_goal_candidate(const goal_lineage* gl, rule_id r) {
    by_goal_.at(gl).insert(r);
}

inline void dbuct_goal_candidate_rules::unlink_goal_candidate(const goal_lineage* gl, rule_id r) {
    by_goal_.at(gl).erase(r);
}

inline void dbuct_goal_candidate_rules::erase(const goal_lineage* gl) {
    auto erased = by_goal_.erase(gl);
    DEBUG_ASSERT(erased == 1);
}

inline void dbuct_goal_candidate_rules::clear_goal_candidate_rule_ids() { by_goal_.clear(); }

inline dbuct_goal_candidate_rules::snapshot_t dbuct_goal_candidate_rules::snapshot() const { return by_goal_; }
inline void dbuct_goal_candidate_rules::restore(snapshot_t s) { by_goal_ = std::move(s); }

#endif
