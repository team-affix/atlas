#ifndef DBUCT_CHOSEN_GOAL_CANDIDATES_HPP
#define DBUCT_CHOSEN_GOAL_CANDIDATES_HPP

#include <memory>
#include <optional>
#include <unordered_map>
#include "infrastructure/backtrackable_map_assign.hpp"
#include "infrastructure/backtrackable_map_insert.hpp"
#include "infrastructure/tracked.hpp"
#include "infrastructure/trail.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

// Delayed-backtracking variant of chosen_goal_candidates.
//
// set() is insert-or-assign, so it journals a map insert for a new goal and a
// map assign (value swap) for an existing one; either way the trail rolls the
// choice back exactly on pop.
struct dbuct_chosen_goal_candidates {
    using map_t = std::unordered_map<const goal_lineage*, rule_id>;

    explicit dbuct_chosen_goal_candidates(trail& t);

    std::optional<rule_id> try_get(const goal_lineage* gl) const;
    void set(const goal_lineage* gl, rule_id r);

private:
    tracked<map_t, trail> by_goal_;
};

inline dbuct_chosen_goal_candidates::dbuct_chosen_goal_candidates(trail& t) : by_goal_(t, map_t{}) {}

inline std::optional<rule_id> dbuct_chosen_goal_candidates::try_get(const goal_lineage* gl) const {
    const auto& m = by_goal_.get();
    const auto it = m.find(gl);
    if (it == m.end()) return std::nullopt;
    return it->second;
}

inline void dbuct_chosen_goal_candidates::set(const goal_lineage* gl, rule_id r) {
    if (by_goal_.get().contains(gl))
        by_goal_.mutate(std::make_unique<backtrackable_map_assign<map_t>>(gl, r));
    else
        by_goal_.mutate(std::make_unique<backtrackable_map_insert<map_t>>(gl, r));
}

#endif
