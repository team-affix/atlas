#ifndef FRONTIER_WATCH_HPP
#define FRONTIER_WATCH_HPP

#include "defs.hpp"
#include "lineage.hpp"
#include "topic.hpp"

struct frontier_watch {
    frontier_watch();
    void resolve(const resolution_lineage*);
#ifndef DEBUG
private:
#endif
    const database& db;
    lineage_pool& lp;
    topic<const goal_lineage*>& goal_inserted_topic;
    topic<const resolution_lineage*>& goal_resolved_topic;
};

#endif