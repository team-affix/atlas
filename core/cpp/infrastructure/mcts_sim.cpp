#include "infrastructure/mcts_sim.hpp"

mcts_sim::mcts_sim(locator& loc, std::mt19937& rng, double exploration_constant)
    : inner_sim_(loc.locate<sim>()),
      rng_(rng),
      exploration_constant_(exploration_constant),
      decision_count_(loc.locate<i_get_decision_count>()),
      mcts_root_() {}

void mcts_sim::set_up() {
    mcts_sim_.emplace(mcts_root_, exploration_constant_, rng_);
    inner_sim_.set_up();
}

void mcts_sim::tear_down() {
    mcts_sim_->terminate(-static_cast<double>(decision_count_.count()));
    inner_sim_.tear_down();
}

mcts_choice mcts_sim::choose(const std::vector<mcts_choice>& choices) {
    return mcts_sim_->choose(choices);
}
