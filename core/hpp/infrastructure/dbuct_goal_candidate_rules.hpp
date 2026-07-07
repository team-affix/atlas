#ifndef DBUCT_GOAL_CANDIDATE_RULES_HPP
#define DBUCT_GOAL_CANDIDATE_RULES_HPP

#include <memory>
#include <unordered_map>
#include "infrastructure/backtrackable_map_at_ra_erase.hpp"
#include "infrastructure/backtrackable_map_at_ra_insert.hpp"
#include "infrastructure/backtrackable_map_erase.hpp"
#include "infrastructure/backtrackable_map_insert.hpp"
#include "infrastructure/ra_rule_id_set_factory.hpp"
#include "infrastructure/ra_rule_id_set.hpp"
#include "infrastructure/tracked.hpp"
#include "value_objects/lineage.hpp"

// Delayed-backtracking variant of goal_candidate_rules. The per-goal active
// candidate set (what CDCL re-application prunes) is carried across episodes and
// rolled back to any choice boundary by the trail (via ILogTrailAction); outer and
// inner mutations each journal a backtrackable mutation, and inner ones re-look-up
// the goal on undo so they survive a node being erased and re-inserted in a frame.
template<typename ILogTrailAction>
struct dbuct_goal_candidate_rules {
    dbuct_goal_candidate_rules(ILogTrailAction& t, ra_rule_id_set_factory& factory);

    const ra_rule_id_set& get(const goal_lineage* gl) const;
    void insert(const goal_lineage* gl);
    void link_goal_candidate(const goal_lineage* gl, rule_id r);
    void unlink_goal_candidate(const goal_lineage* gl, rule_id r);
    void erase(const goal_lineage* gl);

private:
    using map_t = std::unordered_map<const goal_lineage*, ra_rule_id_set>;

    ra_rule_id_set_factory& factory_;
    tracked<map_t, ILogTrailAction> by_goal_;
};

template<typename ILogTrailAction>
dbuct_goal_candidate_rules<ILogTrailAction>::dbuct_goal_candidate_rules(ILogTrailAction& t, ra_rule_id_set_factory& factory)
    : factory_(factory), by_goal_(t, map_t{}) {}

template<typename ILogTrailAction>
const ra_rule_id_set& dbuct_goal_candidate_rules<ILogTrailAction>::get(const goal_lineage* gl) const { return by_goal_.get().at(gl); }

template<typename ILogTrailAction>
void dbuct_goal_candidate_rules<ILogTrailAction>::insert(const goal_lineage* gl) {
    by_goal_.mutate(std::make_unique<backtrackable_map_insert<map_t>>(gl, factory_.make()));
}

template<typename ILogTrailAction>
void dbuct_goal_candidate_rules<ILogTrailAction>::link_goal_candidate(const goal_lineage* gl, rule_id r) {
    by_goal_.mutate(std::make_unique<backtrackable_map_at_ra_insert<map_t>>(gl, r));
}

template<typename ILogTrailAction>
void dbuct_goal_candidate_rules<ILogTrailAction>::unlink_goal_candidate(const goal_lineage* gl, rule_id r) {
    by_goal_.mutate(std::make_unique<backtrackable_map_at_ra_erase<map_t>>(gl, r));
}

template<typename ILogTrailAction>
void dbuct_goal_candidate_rules<ILogTrailAction>::erase(const goal_lineage* gl) {
    by_goal_.mutate(std::make_unique<backtrackable_map_erase<map_t>>(gl));
}

#endif
