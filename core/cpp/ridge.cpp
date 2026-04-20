#include "../hpp/ridge.hpp"
#include "../hpp/ridge_sim.hpp"

ridge::ridge(solver_args sa, mcts_solver_args ma) :
    solver(sa),
    exploration_constant(ma.exploration_constant),
    rng(ma.rng),
    root(),
    mc_sim(std::nullopt)
{}

std::unique_ptr<sim> ridge::construct_sim() {
    mc_sim.emplace(root, exploration_constant, rng);
    return std::make_unique<ridge_sim>(
        sim_args{max_resolutions, db, gl, t, vars, ep, bm, lp, c},
        mcts_sim_args{*mc_sim}
    );
}

void ridge::terminate(sim& s) {
    mc_sim->terminate(-(double)s.get_decisions().size());
}
