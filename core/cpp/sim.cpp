#include "../hpp/sim.hpp"

sim::sim(
    size_t max_resolutions,
    const database& db,
    const goals& goals,
    trail& t,
    sequencer& vars,
    expr_pool& ep,
    bind_map& bm,
    lineage_pool& lp,
    cdcl c
) :
    db(db),
    t(t),
    lp(lp),
    gs(db, goals, t, cp, bm, lp),
    cs(db, goals, lp),
    cp(vars, ep),
    c(c),
    max_resolutions(max_resolutions),
    rs({}),
    ds({})
{}

bool sim::operator()() {

    while ((rs.size() < max_resolutions) && !conflicted() && !solved()) {
        // continue until fixpoint
        if (const resolution_lineage* rl = derive_one()) {
            rs.insert(rl);
            on_resolve(rl);
            continue;
        }

        // decide on a goal and candidate
        const resolution_lineage* rl = decide_one();

        // mark this resolution as a decision
        rs.insert(rl);
        ds.insert(rl);
        on_resolve(rl);
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
    return gs.empty();
}

bool sim::conflicted() {
    // head elimination
    cs.eliminate([this](const goal_lineage* gl, size_t i) { return !gs.applicable(gs.at(gl), db.at(i)); });

    // cdcl elimination
    cs.eliminate([this](const goal_lineage* gl, size_t i) { return c.eliminated(lp.resolution(gl, i)); });

    // we don't need to run in a loop since elimination reaches fixpoint
    return cs.conflicted();
}

const resolution_lineage* sim::derive_one() {
    // unit propagation
    const goal_lineage* propagated_gl;
    size_t propagated_rule_id;

    // if unit, return that resolution
    if (cs.unit(propagated_gl, propagated_rule_id))
        return lp.resolution(propagated_gl, propagated_rule_id);

    // otherwise, return nullptr
    return nullptr;
}

void sim::on_resolve(const resolution_lineage* rl) {
    gs.resolve(rl);
    cs.resolve(rl);
    c.constrain(rl);
}
