#ifndef ELIMINATION_BACKLOG_HPP
#define ELIMINATION_BACKLOG_HPP

#include <unordered_map>
#include <unordered_set>
#include "infrastructure/locator.hpp"
#include "interfaces/i_insert_backlogged_elimination.hpp"
#include "interfaces/i_is_backlogged_elimination.hpp"
#include "interfaces/i_log_to_current_trail_frame.hpp"
#include "infrastructure/tracked.hpp"
#include "value_objects/lineage.hpp"

struct elimination_backlog
    : i_insert_backlogged_elimination
    , i_is_backlogged_elimination {
    elimination_backlog(locator& loc);
    void insert_backlogged_elimination(const resolution_lineage*) override;
    bool is_backlogged_elimination(const resolution_lineage*) const override;
private:
    using eliminated_candidates_type =
        std::unordered_map<const goal_lineage*, std::unordered_set<rule_id>>;
    tracked<eliminated_candidates_type> eliminated_candidates;
};

#endif
