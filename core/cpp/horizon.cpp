#include "../hpp/horizon.hpp"
#include "../hpp/horizon_sim.hpp"
#include "../hpp/sim_args.hpp"
#include "../hpp/topic.hpp"
#include "../hpp/locator.hpp"

horizon::horizon() :
    solver(),
    exploration_constant(locator::locate<double>(locator_keys::inst_mcts_exploration_constant)),
    rng(locator::locate<std::mt19937>(locator_keys::inst_mcts_rng)),
    root(),
    mc_sim(std::nullopt) {
}

std::unique_ptr<sim> horizon::construct_sim() {
    mc_sim.emplace(root, exploration_constant, rng);
    locator::bind(locator_keys::inst_mcts_sim, *mc_sim);
    return std::make_unique<horizon_sim>();
}

void horizon::terminate(sim& s) {
    mc_sim->terminate(static_cast<horizon_sim&>(s).reward());
}
