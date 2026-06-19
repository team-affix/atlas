#include "infrastructure/chosen_goal_candidates.hpp"

std::optional<rule_id> chosen_goal_candidates::try_get(const goal_lineage* gl) const {
    const auto it = by_goal_.find(gl);
    if (it == by_goal_.end())
        return std::nullopt;
    return it->second;
}

void chosen_goal_candidates::set(const goal_lineage* gl, rule_id r) {
    by_goal_[gl] = r;
}

void chosen_goal_candidates::clear() {
    by_goal_.clear();
}
