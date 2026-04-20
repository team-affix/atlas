#ifndef MCTS_SOLVER_ARGS_HPP
#define MCTS_SOLVER_ARGS_HPP

#include <random>

struct mcts_solver_args {
    double        exploration_constant;
    std::mt19937& rng;
};

#endif
