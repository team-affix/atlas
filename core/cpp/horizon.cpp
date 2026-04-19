#include "../hpp/horizon.hpp"
#include "../hpp/horizon_sim.hpp"

horizon::horizon(solver_context ctx, mcts_params mp) :
    solver(ctx),
    exploration_constant(mp.exploration_constant),
    rng(mp.rng),
    root(),
    mc_sim(std::nullopt)
{}

std::unique_ptr<sim> horizon::construct_sim() {
    mc_sim.emplace(root, exploration_constant, rng);
    return std::make_unique<horizon_sim>(
        sim_context{max_resolutions, db, gl, t, vars, ep, bm, lp, c}, *mc_sim);
}

void horizon::terminate(sim& s) {
    mc_sim->terminate(static_cast<horizon_sim&>(s).reward());
}
