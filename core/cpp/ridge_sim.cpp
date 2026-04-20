#include "../hpp/ridge_sim.hpp"

ridge_sim::ridge_sim(sim_args sa, mcts_sim_args ma) :
    sim(sa),
    dec(cs, ma.mc_sim)
{}

const resolution_lineage* ridge_sim::decide_one() {
    auto [chosen_goal, chosen_candidate] = dec();
    return lp.resolution(chosen_goal, chosen_candidate);
}

void ridge_sim::on_resolve(const resolution_lineage*) {}
