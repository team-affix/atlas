#ifndef FRONTIER_WATCH_HPP
#define FRONTIER_WATCH_HPP

#include "../../value_objects/lineage.hpp"
#include "topic.hpp"

struct goal_resolver {
    goal_resolver();
    void goal_resolved(const resolution_lineage*);
#ifndef DEBUG
private:
#endif
    const database& db;
    lineage_pool& lp;
    topic<const goal_lineage*>& goal_inserted_topic;
};

#endif