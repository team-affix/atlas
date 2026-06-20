#ifndef ELIMINATION_BACKLOG_HPP
#define ELIMINATION_BACKLOG_HPP

#include <unordered_map>
#include <unordered_set>
#include "infrastructure/tracked.hpp"
#include "infrastructure/trail.hpp"
#include "infrastructure/backtrackable_map_insert.hpp"
#include "infrastructure/backtrackable_map_at_insert.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

struct elimination_backlog {
    explicit elimination_backlog(trail&);
    void insert_backlogged_elimination(const resolution_lineage*);
    bool is_backlogged_elimination(const resolution_lineage*) const;
private:
    using eliminated_candidates_type =
        std::unordered_map<const goal_lineage*, std::unordered_set<rule_id>>;
    tracked<eliminated_candidates_type, trail> eliminated_candidates;
};

#endif
