#include "../hpp/sim.hpp"

sim::sim(sim_args args) :
    db(args.db),
    t(args.t),
    lp(args.lp),
    gs(args.db, args.gl, args.t, cp, args.bm, args.lp),
    cs(args.db, args.gl, args.lp),
    cp(args.vars, args.ep),
    c(args.c),
    ea(db, args.gl, args.bm, args.ep, gs, cs, lp, c),
    rs(),
    ds(),
    max_resolutions(args.max_resolutions)
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
    return gs.get().empty();
}

bool sim::conflicted() {
    return ea();
}

const resolution_lineage* sim::derive_one() {
    return ea.pop_unit();
}

void sim::resolve(const resolution_lineage* rl) {
    rs.insert(rl);
    gs.resolve(rl);
    cs.resolve(rl);
    c.constrain(rl);
    ea.resolve(rl);
    on_resolve(rl);
}
