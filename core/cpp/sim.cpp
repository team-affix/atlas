#include "../hpp/sim.hpp"

sim::sim(trail& t, size_t max_resolutions) :
    max_resolutions(max_resolutions),
    rs(t, {}),
    ds(t, {})
{}

bool sim::operator()() {
    
    while ((rs.get().size() < max_resolutions) && !conflicted() && !solved()) {
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
    return rs.get();
}

const decisions& sim::get_decisions() const {
    return ds.get();
}
