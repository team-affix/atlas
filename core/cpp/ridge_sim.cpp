#include "../hpp/ridge_sim.hpp"

ridge_sim::ridge_sim(
    sim_context ctx,
    monte_carlo::simulation<mcts_decider::choice, std::mt19937>& mc_sim
) :
    sim(ctx),
    dec(cs, mc_sim)
{}

const resolution_lineage* ridge_sim::decide_one() {
    auto [chosen_goal, chosen_candidate] = dec();
    return lp.resolution(chosen_goal, chosen_candidate);
}

void ridge_sim::on_resolve(const resolution_lineage*) {}
