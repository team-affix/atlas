#include "../hpp/frontier_watch.hpp"
#include "../hpp/locator.hpp"

frontier_watch::frontier_watch() :
    db(locator::locate<database>(locator_keys::inst_database)),
    lp(locator::locate<lineage_pool>(locator_keys::inst_lineage_pool)),
    goal_inserted_topic(locator::locate<topic<const goal_lineage*>>(locator_keys::inst_goal_inserted_topic)),
    goal_resolved_topic(locator::locate<topic<const resolution_lineage*>>(locator_keys::inst_goal_resolved_topic)) {
    const goals& gl = locator::locate<goals>(locator_keys::inst_goals);
    for (int i = 0; i < gl.size(); ++i)
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
