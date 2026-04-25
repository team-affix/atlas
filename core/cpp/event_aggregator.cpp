#include "../hpp/event_aggregator.hpp"

event_aggregator::event_aggregator(
    const database& db,
    const goals& gls,
    bind_map& bm,
    expr_pool& ep,
    goal_store& gs,
    candidate_store& cs,
    lineage_pool& lp,
    cdcl& c
) :
    lp(lp),
    cs(cs),
    conflict_register(false),
    unit_queue(),
    fw(db, lp),
    ce(db, gls, ep, cs, lp, c, conflict_register, unit_queue),
    he(db, gls, bm, ep, gs, cs, lp, conflict_register, unit_queue) {
    fw.set_insert_callback(goal_inserted_callback());
    fw.set_resolve_callback(goal_resolved_callback());
    fw.initialize(gls);
}

bool event_aggregator::operator()() {
    if (conflict_register)
        return true;
    ce();
    if (conflict_register)
        return true;
    he();
    return conflict_register;
}

bool event_aggregator::pop_unit(const resolution_lineage*& rl) {
    if (unit_queue.empty())
        return false;
    rl = unit_queue.front();
    unit_queue.pop();
    return true;
}

void event_aggregator::resolve(const resolution_lineage* rl) {
    fw.resolve(rl);
    ce.resolve(rl);
    he.resolve(rl);
}

std::function<void(const goal_lineage*)> event_aggregator::goal_inserted_callback() {
    return [this](const goal_lineage* gl) {
        const auto& candidates = cs.at(gl);
        if (candidates.empty())
            conflict_register = true;
        else if (candidates.size() == 1)
            unit_queue.push(lp.resolution(gl, *candidates.begin()));
    };
}

std::function<void(const resolution_lineage*)> event_aggregator::goal_resolved_callback() {
    return [this](const resolution_lineage*) {};
}
