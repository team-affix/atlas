#ifndef CDCL_ELIMINATOR_HPP
#define CDCL_ELIMINATOR_HPP

#include "../value_objects/lineage.hpp"
#include "../candidate_store/candidate_store.hpp"
#include "../../../infrastructure/event_topic.hpp"
#include "../../events/unit_event.hpp"

struct cdcl_eliminator {
    cdcl_eliminator();
    bool operator()();
    void cdcl_eliminated_candidate(const resolution_lineage*);
    void goal_activated(const goal_lineage*);
    void goal_deactivated(const goal_lineage*);
#ifndef DEBUG
private:
#endif
    bool route_elimination(const resolution_lineage*);
    bool flush_backlog_for_goal(const goal_lineage*);
    bool eliminate(const goal_lineage*, size_t);
    
    lineage_pool& lp;
    candidate_store& cs;
    event_topic<unit_event>& unit_topic;

    std::unordered_set<const goal_lineage*> active_goals;
    std::unordered_set<const goal_lineage*> resolved_goals;
    std::unordered_map<const goal_lineage*, std::unordered_set<size_t>> elimination_backlog;
};

#endif
