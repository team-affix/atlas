#include "../hpp/horizon_sim.hpp"

horizon_sim::horizon_sim(sim_args sa, mcts_sim_args ma) :
    sim(sa),
    dec(cs, ma.mc_sim),
    ws(sa.gl, sa.db, sa.lp)
{}

double horizon_sim::reward() {
    return ws.total();
}

const resolution_lineage* horizon_sim::decide_one() {
    auto [chosen_goal, chosen_candidate] = dec();
    return lp.resolution(chosen_goal, chosen_candidate);
}

void horizon_sim::on_resolve(const resolution_lineage* rl) {
    ws.resolve(rl);
}
