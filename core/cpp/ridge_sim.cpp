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
    sim(max_resolutions, db, goals, t, vars, ep, bm, lp, c),
    dec(cs, mc_sim)
{}

const resolution_lineage* ridge_sim::decide_one() {
    auto [chosen_goal, chosen_candidate] = dec();
    return lp.resolution(chosen_goal, chosen_candidate);
}
