#include "../hpp/ridge.hpp"
#include "../hpp/ridge_sim.hpp"
#include "../hpp/locator.hpp"

ridge::ridge() :
    solver(),
    exploration_constant(locator::locate<double>(locator_keys::inst_mcts_exploration_constant)),
    rng(locator::locate<std::mt19937>(locator_keys::inst_mcts_rng)),
    root(),
    mc_sim(std::nullopt) {
    max_resolutions = 1000;
}

std::unique_ptr<sim> ridge::construct_sim() {
    mc_sim.emplace(root, exploration_constant, rng);
    locator::bind(locator_keys::inst_mcts_sim, *mc_sim);
    return std::make_unique<ridge_sim>();
}

void ridge::terminate(sim& s) {
    mc_sim->terminate(-(double)s.get_decisions().size());
}
