#include "../hpp/sim.hpp"

sim::sim(sim_context ctx) :
    db(ctx.db),
    t(ctx.t),
    lp(ctx.lp),
    gs(ctx.db, ctx.gl, ctx.t, cp, ctx.bm, ctx.lp),
    cs(ctx.db, ctx.gl, ctx.lp),
    cp(ctx.vars, ctx.ep),
    c(ctx.c),
    max_resolutions(ctx.max_resolutions),
    rs({}),
    ds({})
{}

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

void sim::resolve(const resolution_lineage* rl) {
    rs.insert(rl);
    gs.resolve(rl);
    cs.resolve(rl);
    c.constrain(rl);
    on_resolve(rl);
}
