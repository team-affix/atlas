#include "../hpp/sim.hpp"
#include "../hpp/locator.hpp"

sim::sim() :
    db(locator::locate<database>(locator_keys::inst_database)),
    t(locator::locate<trail>(locator_keys::inst_trail)),
    lp(locator::locate<lineage_pool>(locator_keys::inst_lineage_pool)),
    gs(),
    cs(),
    cp(),
    c(),
    goal_inserted_topic(),
    goal_resolved_topic(),
    new_eliminated_resolution_topic(),
    unit_topic(),
    unit_subscription(unit_topic),
    icd(),
    ce(),
    he(),
    fw(),
    rs(),
    ds(),
    max_resolutions(0) {
}

bool sim::operator()() {

    while ((rs.size() < max_resolutions) && !conflicted() && !solved()) {
        // continue until fixpoint
        if (const resolution_lineage* rl = derive_one()) {
            resolve(rl);
            continue;
        }

        // decide on a goal and candidate
        const resolution_lineage* rl = decide_one();

        // mark this resolution as a decision
        ds.insert(rl);
        resolve(rl);
    }

    // return whether a solution was found
    return solved();
}

const resolutions& sim::get_resolutions() const {
    return rs;
}

const decisions& sim::get_decisions() const {
    return ds;
}

bool sim::solved() {
    return gs.get().empty();
}

bool sim::conflicted() {
    return icd() || ce() || he();
}

const resolution_lineage* sim::derive_one() {
    if (unit_subscription.empty())
        return nullptr;
    return unit_subscription.consume();
}

void sim::resolve(const resolution_lineage* rl) {
    rs.insert(rl);
    fw.resolve(rl);
    gs.resolve(rl);
    cs.resolve(rl);
    c.constrain(rl);
    on_resolve(rl);
}
