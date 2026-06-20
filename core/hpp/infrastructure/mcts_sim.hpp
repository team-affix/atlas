#ifndef MCTS_SIM_HPP
#define MCTS_SIM_HPP

#include <optional>
#include <random>
#include <vector>
#include "value_objects/mcts_choice.hpp"
#include "mcts.hpp"

template<typename ISetUpSim, typename ITearDownSim, typename IComputeMctsReward>
struct mcts_sim {
    mcts_sim(ISetUpSim&, ITearDownSim&, IComputeMctsReward&,
             std::mt19937&, double exploration_constant);
    void set_up();
    void tear_down();
    mcts_choice choose(const std::vector<mcts_choice>&);
private:
    ISetUpSim& set_up_;
    ITearDownSim& tear_down_;
    IComputeMctsReward& compute_mcts_reward_;
    std::mt19937& rng_;
    double exploration_constant_;
    monte_carlo::tree_node<mcts_choice> mcts_root_;
    std::optional<monte_carlo::simulation<mcts_choice, std::mt19937>> mcts_sim_;
};

template<typename ISUS, typename ITDS, typename ICMR>
mcts_sim<ISUS,ITDS,ICMR>::mcts_sim(
    ISUS& sus, ITDS& tds, ICMR& cmr, std::mt19937& rng, double ec)
    : set_up_(sus), tear_down_(tds), compute_mcts_reward_(cmr),
      rng_(rng), exploration_constant_(ec), mcts_root_() {}

template<typename ISUS, typename ITDS, typename ICMR>
void mcts_sim<ISUS,ITDS,ICMR>::set_up() {
    mcts_sim_.emplace(mcts_root_, exploration_constant_, rng_);
    set_up_.set_up();
}

template<typename ISUS, typename ITDS, typename ICMR>
void mcts_sim<ISUS,ITDS,ICMR>::tear_down() {
    mcts_sim_->terminate(compute_mcts_reward_.compute_mcts_reward());
    tear_down_.tear_down();
}

template<typename ISUS, typename ITDS, typename ICMR>
mcts_choice mcts_sim<ISUS,ITDS,ICMR>::choose(const std::vector<mcts_choice>& choices) {
    return mcts_sim_->choose(choices);
}

#endif
