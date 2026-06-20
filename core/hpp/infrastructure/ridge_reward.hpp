#ifndef RIDGE_REWARD_HPP
#define RIDGE_REWARD_HPP

template<typename IDecisionMemory>
struct ridge_reward {
    ridge_reward(IDecisionMemory& dm);
    double compute_mcts_reward() const;
private:
    IDecisionMemory& decision_count_;
};

template<typename IDecisionMemory>
ridge_reward<IDecisionMemory>::ridge_reward(IDecisionMemory& dm) : decision_count_(dm) {}

template<typename IDecisionMemory>
double ridge_reward<IDecisionMemory>::compute_mcts_reward() const {
    return -static_cast<double>(decision_count_.count());
}

#endif
