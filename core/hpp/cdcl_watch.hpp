#ifndef CDCL_WATCH_HPP
#define CDCL_WATCH_HPP

#include "cdcl.hpp"
#include "lineage.hpp"

struct cdcl_watch {
    cdcl_watch(cdcl&, lineage_pool&);
    void goal_added(const goal_lineage*);
    void goal_resolved(const goal_lineage*);
    void pipe();
    std::queue<const resolution_lineage*> eliminated_frontier_resolutions;
#ifndef DEBUG
private:
#endif
    cdcl& c;
    lineage_pool& lp;
    
    std::unordered_set<const goal_lineage*> frontier_goals;
    std::unordered_map<const goal_lineage*, std::unordered_set<size_t>> elimination_backlog;
};

#endif
