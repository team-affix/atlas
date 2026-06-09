#ifndef MCTS_SIM_HPP
#define MCTS_SIM_HPP

#include <optional>
#include <random>
#include "infrastructure/locator.hpp"
#include "infrastructure/set_up_sim.hpp"
#include "infrastructure/tear_down_sim.hpp"
#include "interfaces/i_set_up_sim.hpp"
#include "interfaces/i_tear_down_sim.hpp"
#include "interfaces/i_mcts_choose.hpp"
#include "interfaces/i_get_decision_count.hpp"
#include "value_objects/mcts_choice.hpp"
#include "mcts.hpp"

struct mcts_sim
    : i_set_up_sim
    , i_tear_down_sim
    , i_mcts_choose {
    mcts_sim(
        locator& loc,
        set_up_sim& set_up,
        tear_down_sim& tear_down,
        std::mt19937& rng,
        double exploration_constant);

    void set_up() override;
    void tear_down() override;
    mcts_choice choose(const std::vector<mcts_choice>&) override;

private:
    set_up_sim& set_up_;
    tear_down_sim& tear_down_;
    i_get_decision_count& decision_count_;

    std::mt19937& rng_;
    double exploration_constant_;

    monte_carlo::tree_node<mcts_choice> mcts_root_;
    std::optional<monte_carlo::simulation<mcts_choice, std::mt19937>> mcts_sim_;
};

#endif
