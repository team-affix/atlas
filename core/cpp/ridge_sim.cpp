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
    cdcl c,
    monte_carlo::simulation<mcts_decider::choice, std::mt19937>& mc_sim
) :
    sim(max_resolutions),
    db(db),
    t(t),
    lp(lp),
    gs(db, goals, t, cp, bm, lp),
    cs(db, goals, lp),
    cp(vars, ep),
    dec(cs, mc_sim),
    c(c)
{}

bool ridge_sim::solved() {
    return gs.empty();
}

bool ridge_sim::conflicted() {
    // head elimination
    cs.eliminate([this](const goal_lineage* gl, size_t i) { return !gs.applicable(gs.at(gl), db.at(i)); });

    // cdcl elimination
    cs.eliminate([this](const goal_lineage* gl, size_t i) { return c.eliminated(lp.resolution(gl, i)); });

    // we don't need to run in a loop since elimination reaches fixpoint
    return cs.conflicted();
}

const resolution_lineage* ridge_sim::derive_one() {
    // unit propagation
    const goal_lineage* propagated_gl;
    size_t propagated_rule_id;

    // if unit, return that resolution
    if (cs.unit(propagated_gl, propagated_rule_id))
        return lp.resolution(propagated_gl, propagated_rule_id);
    
    // otherwise, return nullptr
    return nullptr;
}

const resolution_lineage* ridge_sim::decide_one() {
    // decide on a goal and candidate
    auto [chosen_goal, chosen_candidate] = dec();

    // construct the resolution lineage
    return lp.resolution(chosen_goal, chosen_candidate);
}

void ridge_sim::on_resolve(const resolution_lineage* rl) {
    gs.resolve(rl);
    cs.resolve(rl);
    c.constrain(rl);
}
