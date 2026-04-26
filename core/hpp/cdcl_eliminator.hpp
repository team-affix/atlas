#ifndef CDCL_ELIMINATOR_HPP
#define CDCL_ELIMINATOR_HPP

#include <queue>
#include "cdcl.hpp"
#include "lineage.hpp"
#include "candidate_store.hpp"
#include "frontier_watch.hpp"
#include "topic.hpp"

struct cdcl_eliminator {
    cdcl_eliminator(
        candidate_store&,
        lineage_pool&,
        cdcl&,
        topic<const goal_lineage*>&,
        topic<const resolution_lineage*>&,
        topic<const resolution_lineage*>&,
        topic<const resolution_lineage*>&
    );
    bool operator()();
#ifndef DEBUG
private:
#endif
    bool route_elimination(const resolution_lineage*);
    bool flush_backlog_for_goal(const goal_lineage*);
    bool eliminate(const goal_lineage*, size_t);
    bool flush_new_eliminated_resolutions();
    bool flush_goal_inserted();
    void flush_goal_resolved();
    
    lineage_pool& lp;
    candidate_store& cs;
    topic<const goal_lineage*>::subscription goal_inserted_subscription;
    topic<const resolution_lineage*>::subscription goal_resolved_subscription;
    topic<const resolution_lineage*>::subscription new_eliminated_resolution_subscription;
    topic<const resolution_lineage*>& unit_topic;

    std::unordered_set<const goal_lineage*> active_goals;
    std::unordered_set<const goal_lineage*> resolved_goals;
    std::unordered_map<const goal_lineage*, std::unordered_set<size_t>> elimination_backlog;
};

#endif
