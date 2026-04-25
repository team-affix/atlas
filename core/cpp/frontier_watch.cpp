#include "../hpp/frontier_watch.hpp"

frontier_watch::frontier_watch(
    const database& db,
    lineage_pool& lp) : db(db), lp(lp) {
}

void frontier_watch::initialize(const goals& goals) {
    for (int i = 0; i < goals.size(); ++i)
        insert_callback(lp.goal(nullptr, i));
}

void frontier_watch::resolve(const resolution_lineage* r) {
    // get the parent
    const goal_lineage* parent = r->parent;

    // get the rule from db
    const rule& db_rule = db.at(r->idx);

    // add children
    for (int i = 0; i < db_rule.body.size(); ++i)
        insert_callback(lp.goal(r, i));

    // notify the callback
    resolve_callback(r);
}

void frontier_watch::set_insert_callback(std::function<void(const goal_lineage*)> callback) {
    insert_callback = callback;
}

void frontier_watch::set_resolve_callback(std::function<void(const resolution_lineage*)> callback) {
    resolve_callback = callback;
}
