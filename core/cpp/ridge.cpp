#include "../hpp/ridge.hpp"
#include "../hpp/ridge_sim.hpp"

ridge::ridge(solver_context ctx, mcts_params mp) :
    solver(ctx),
    exploration_constant(mp.exploration_constant),
    rng(mp.rng),
    root(),
    mc_sim(std::nullopt)
{}

std::unique_ptr<sim> ridge::construct_sim() {
    mc_sim.emplace(root, exploration_constant, rng);
    return std::make_unique<ridge_sim>(
        sim_context{max_resolutions, db, gl, t, vars, ep, bm, lp, c}, *mc_sim);
}

void ridge::terminate(sim& s) {
    mc_sim->terminate(-(double)s.get_decisions().size());
}
