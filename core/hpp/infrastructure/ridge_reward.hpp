#ifndef RIDGE_REWARD_HPP
#define RIDGE_REWARD_HPP

#include "infrastructure/locator.hpp"
#include "interfaces/i_compute_mcts_reward.hpp"
#include "interfaces/i_get_decision_count.hpp"

struct ridge_reward : i_compute_mcts_reward {
    ridge_reward(locator& loc);
    double compute_mcts_reward() const override;

private:
    i_get_decision_count& decision_count_;
};

#endif
