#ifndef I_COMPUTE_MCTS_REWARD_HPP
#define I_COMPUTE_MCTS_REWARD_HPP

struct i_compute_mcts_reward {
    virtual ~i_compute_mcts_reward() = default;
    virtual double compute_mcts_reward() const = 0;
};

#endif
