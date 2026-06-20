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

#endif
