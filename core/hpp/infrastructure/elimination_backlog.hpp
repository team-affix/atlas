#ifndef ELIMINATION_BACKLOG_HPP
#define ELIMINATION_BACKLOG_HPP

#include <unordered_map>
#include <unordered_set>
#include "interfaces/i_insert_backlogged_elimination.hpp"
#include "interfaces/i_is_backlogged_elimination.hpp"
#include "interfaces/i_constrain_elimination_backlog.hpp"
#include "value_objects/lineage.hpp"

struct elimination_backlog
    : i_insert_backlogged_elimination
    , i_is_backlogged_elimination
    , i_constrain_elimination_backlog {
    void insert_backlogged_elimination(const resolution_lineage*) override;
    bool is_backlogged_elimination(const resolution_lineage*) const override;
    void constrain_elimination_backlog(const resolution_lineage*) override;
private:
    std::unordered_map<const goal_lineage*, std::unordered_set<rule_id>> eliminated_candidates_;
};

#endif
