#ifndef RIDGE_HPP
#define RIDGE_HPP

#include <optional>
#include <random>
#include "solver.hpp"
#include "mcts_decider.hpp"
#include "../../mcts/include/mcts.hpp"

struct ridge : solver {
    ridge();
#ifndef DEBUG
protected:
#endif
    std::unique_ptr<sim> construct_sim() override;
    void terminate(sim&) override;

    double exploration_constant;
    std::mt19937& rng;
    monte_carlo::tree_node<mcts_decider::choice> root;
    std::optional<monte_carlo::simulation<mcts_decider::choice, std::mt19937>> mc_sim;
};

#endif
