#include "../hpp/ridge_sim.hpp"

ridge_sim::ridge_sim(
    size_t max_resolutions,
    const database& db,
    const goals& goals,
    trail& t,
    sequencer& vars,
    expr_pool& ep,
    bind_map& bm,
    lineage_pool& lp,
    resolutions& rs,
    decisions& ds,
    cdcl c,
    monte_carlo::simulation<mcts_decider::choice, std::mt19937>& sim
) :
    max_resolutions(max_resolutions),
    db(db),
    t(t),
    lp(lp),
    rs(rs),
    ds(ds),
    gs(db, goals, t, cp, bm, lp),
    cs(db, goals, lp),
    cp(vars, ep),
    dec(cs, sim),
    c(c)
{
    // clear the resolution and decision stores
    // for the start of the simulation
    rs.clear();
    ds.clear();
}

bool ridge_sim::operator()() {

    while ((rs.size() < max_resolutions) && !cs.conflicted() && !gs.empty()) {

        // head elimination
        size_t elim0 = cs.eliminate([this](const goal_lineage* gl, size_t i) { return !gs.applicable(gs.at(gl), db.at(i)); });

        // cdcl elimination
        size_t elim1 = cs.eliminate([this](const goal_lineage* gl, size_t i) { return c.eliminated(lp.resolution(gl, i)); });
        
        // unit propagation
        const goal_lineage* propagated_gl;
        size_t propagated_rule_id;
        bool unit = cs.unit(propagated_gl, propagated_rule_id);

        // if a unit propagation is found, enact it
        if (unit)
            resolve(lp.resolution(propagated_gl, propagated_rule_id));

        // continue until fixpoint
        if (elim0 > 0 || elim1 > 0 || unit)
            continue;

        // decide on a goal and candidate
        auto [chosen_goal, chosen_candidate] = dec();

        // construct the resolution lineage
        const resolution_lineage* rl = lp.resolution(chosen_goal, chosen_candidate);

        // resolve the chosen goal and candidate
        resolve(rl);
        
        // mark this resolution as a decision
        ds.insert(rl);

    }

    // return whether a solution was found
    return gs.empty();
}

void ridge_sim::resolve(const resolution_lineage* rl) {
    gs.resolve(rl);
    cs.resolve(rl);
    c.constrain(rl);
    rs.insert(rl);
}
