#ifndef MCTS_SIM_ARGS_HPP
#define MCTS_SIM_ARGS_HPP

#include <random>
#include "../../mcts/include/mcts.hpp"
#include "mcts_decider.hpp"

struct mcts_sim_args {
    monte_carlo::simulation<mcts_decider::choice, std::mt19937>& mc_sim;
};

#endif
