#include "../hpp/frontier_watch.hpp"

frontier_watch::frontier_watch(
    const database& db,
    lineage_pool& lp,
    topic<const goal_lineage*>& goal_inserted_topic,
    topic<const resolution_lineage*>& goal_resolved_topic)
    :
    db(db),
    lp(lp), 
    goal_inserted_topic(goal_inserted_topic),
    goal_resolved_topic(goal_resolved_topic) {
}

void frontier_watch::initialize(const goals& goals) {
    for (int i = 0; i < goals.size(); ++i)
        goal_inserted_topic.produce(lp.goal(nullptr, i));
}

void frontier_watch::resolve(const resolution_lineage* r) {
    // get the parent
    const goal_lineage* parent = r->parent;

    // get the rule from db
    const rule& db_rule = db.at(r->idx);

    // add children
    for (int i = 0; i < db_rule.body.size(); ++i)
        goal_inserted_topic.produce(lp.goal(r, i));

    // notify the callback
    goal_resolved_topic.produce(r);
}
