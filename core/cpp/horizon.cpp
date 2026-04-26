#include "../hpp/horizon.hpp"
#include "../hpp/horizon_sim.hpp"
#include "../hpp/sim_args.hpp"
#include "../hpp/topic.hpp"
#include "../hpp/locator.hpp"

horizon::horizon(solver_args sa, mcts_solver_args ma) :
    solver(),
    exploration_constant(ma.exploration_constant),
    rng(ma.rng),
    root(),
    mc_sim(std::nullopt) {
    max_resolutions = sa.max_resolutions;
}

std::unique_ptr<sim> horizon::construct_sim() {
    mc_sim.emplace(root, exploration_constant, rng);
    locator::bind(locator_keys::inst_mcts_sim, *mc_sim);
    return std::make_unique<horizon_sim>();
}

void horizon::terminate(sim& s) {
    mc_sim->terminate(static_cast<horizon_sim&>(s).reward());
}
