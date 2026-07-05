#ifndef DBUCT_CHOSEN_GOAL_CANDIDATES_HPP
#define DBUCT_CHOSEN_GOAL_CANDIDATES_HPP

#include <optional>
#include <unordered_map>
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

// Delayed-backtracking variant of chosen_goal_candidates.
struct dbuct_chosen_goal_candidates {
    using snapshot_t = std::unordered_map<const goal_lineage*, rule_id>;

    std::optional<rule_id> try_get(const goal_lineage* gl) const;
    void set(const goal_lineage* gl, rule_id r);

    snapshot_t snapshot() const;
    void restore(snapshot_t s);

private:
    std::unordered_map<const goal_lineage*, rule_id> by_goal_;
};

inline std::optional<rule_id> dbuct_chosen_goal_candidates::try_get(const goal_lineage* gl) const {
    const auto it = by_goal_.find(gl);
    if (it == by_goal_.end()) return std::nullopt;
    return it->second;
}

inline void dbuct_chosen_goal_candidates::set(const goal_lineage* gl, rule_id r) { by_goal_[gl] = r; }

inline dbuct_chosen_goal_candidates::snapshot_t dbuct_chosen_goal_candidates::snapshot() const { return by_goal_; }
inline void dbuct_chosen_goal_candidates::restore(snapshot_t s) { by_goal_ = std::move(s); }

#endif
