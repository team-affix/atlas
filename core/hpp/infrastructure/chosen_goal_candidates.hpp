#ifndef CHOSEN_GOAL_CANDIDATES_HPP
#define CHOSEN_GOAL_CANDIDATES_HPP

#include <optional>
#include <unordered_map>
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

struct chosen_goal_candidates {
    std::optional<rule_id> try_get(const goal_lineage*) const;
    void set(const goal_lineage*, rule_id);
    void clear();
private:
    std::unordered_map<const goal_lineage*, rule_id> by_goal_;
};

inline std::optional<rule_id> chosen_goal_candidates::try_get(const goal_lineage* gl) const {
    const auto it = by_goal_.find(gl);
    if (it == by_goal_.end()) return std::nullopt;
    return it->second;
}

inline void chosen_goal_candidates::set(const goal_lineage* gl, rule_id r) {
    by_goal_[gl] = r;
}

inline void chosen_goal_candidates::clear() {
    by_goal_.clear();
}

#endif
