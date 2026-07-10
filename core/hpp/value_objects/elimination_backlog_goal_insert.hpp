#ifndef ELIMINATION_BACKLOG_GOAL_INSERT_HPP
#define ELIMINATION_BACKLOG_GOAL_INSERT_HPP

#include <compare>
#include <unordered_set>
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

struct elimination_backlog_goal_insert {
    const goal_lineage* gl;
    std::unordered_set<rule_id> value;
    auto operator<=>(const elimination_backlog_goal_insert&) const = default;
};

#endif
