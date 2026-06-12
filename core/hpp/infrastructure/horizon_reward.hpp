#ifndef HORIZON_REWARD_HPP
#define HORIZON_REWARD_HPP

#include "infrastructure/locator.hpp"
#include "interfaces/i_compute_mcts_reward.hpp"
#include "interfaces/i_get_grounded_weight.hpp"

struct horizon_reward : i_compute_mcts_reward {
    horizon_reward(locator& loc);
    double compute_mcts_reward() const override;
private:
    i_get_grounded_weight& grounded_weight_;
};

#endif
