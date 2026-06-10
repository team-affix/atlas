#include "infrastructure/mcts_sim.hpp"

mcts_sim::mcts_sim(
    locator& loc,
    set_up_sim& set_up,
    tear_down_sim& tear_down,
    std::mt19937& rng,
    double exploration_constant)
    : set_up_(set_up),
      tear_down_(tear_down),
      rng_(rng),
      exploration_constant_(exploration_constant),
      compute_mcts_reward_(loc.locate<i_compute_mcts_reward>()),
      mcts_root_() {}

void mcts_sim::set_up() {
    mcts_sim_.emplace(mcts_root_, exploration_constant_, rng_);
    set_up_.set_up();
}

void mcts_sim::tear_down() {
    mcts_sim_->terminate(compute_mcts_reward_.compute_mcts_reward());
    tear_down_.tear_down();
}

mcts_choice mcts_sim::choose(const std::vector<mcts_choice>& choices) {
    return mcts_sim_->choose(choices);
}
