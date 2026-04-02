#include "../hpp/horizon_sim.hpp"

horizon_sim::horizon_sim(
    size_t max_resolutions,
    const database& db,
    const goals& goals,
    trail& t,
    sequencer& seq,
    expr_pool& ep,
    bind_map& bm,
    lineage_pool& lp,
    cdcl c,
    monte_carlo::simulation<mcts_decider::choice, std::mt19937>& mc_sim
) :
    ridge_sim(max_resolutions, db, goals, t, seq, ep, bm, lp, c, mc_sim),
    ws(goals, db, lp)
{}

double horizon_sim::reward() {
    return ws.total();
}

void horizon_sim::on_resolve(const resolution_lineage* rl) {
    ridge_sim::on_resolve(rl);
    ws.resolve(rl);
}
