#ifndef CDCL_ELIMINATOR_HPP
#define CDCL_ELIMINATOR_HPP

#include "cdcl.hpp"
#include "lineage.hpp"
#include "candidate_store.hpp"
#include "frontier_watch.hpp"
#include <queue>

struct cdcl_eliminator {
    cdcl_eliminator(
        const database&,
        const goals&,
        expr_pool&,
        candidate_store&,
        lineage_pool&,
        cdcl&,
        std::queue<const resolution_lineage*>&
    );
    bool operator()();
    void resolve(const resolution_lineage*);
#ifndef DEBUG
private:
#endif
    void route_elimination(const resolution_lineage*);
    void flush_backlog_for_goal(const goal_lineage*);
    void eliminate(const goal_lineage*, size_t);
    void check_unit(const goal_lineage*);
    void check_conflict(const goal_lineage*);
    
    std::function<void(const resolution_lineage*)> new_eliminated_resolution_callback();
    std::function<void(const goal_lineage*)> goal_inserted_callback();
    std::function<void(const resolution_lineage*)> goal_resolved_callback();

    lineage_pool& lp;
    candidate_store& cs;
    std::queue<const resolution_lineage*>& unit_queue;

    frontier_watch fw;
    std::unordered_set<const goal_lineage*> active_goals;
    std::unordered_set<const goal_lineage*> resolved_goals;
    std::queue<const resolution_lineage*> new_eliminated_resolutions;
    std::unordered_map<const goal_lineage*, std::unordered_set<size_t>> elimination_backlog;
    bool conflict_register;
};

#endif
