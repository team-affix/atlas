#ifndef ELIMINATION_BACKLOG_CANDIDATE_INSERT_HPP
#define ELIMINATION_BACKLOG_CANDIDATE_INSERT_HPP

#include <compare>
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

struct elimination_backlog_candidate_insert {
    const goal_lineage* gl;
    rule_id candidate;
    auto operator<=>(const elimination_backlog_candidate_insert&) const = default;
};

#endif
