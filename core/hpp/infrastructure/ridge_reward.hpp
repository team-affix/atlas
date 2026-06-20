#ifndef RIDGE_REWARD_HPP
#define RIDGE_REWARD_HPP

template<typename IGetDecisionCount>
struct ridge_reward {
    ridge_reward(IGetDecisionCount& dm);
    double compute_mcts_reward() const;
private:
    IGetDecisionCount& decision_count_;
};

template<typename IGetDecisionCount>
ridge_reward<IGetDecisionCount>::ridge_reward(IGetDecisionCount& dm) : decision_count_(dm) {}

template<typename IGetDecisionCount>
double ridge_reward<IGetDecisionCount>::compute_mcts_reward() const {
    return -static_cast<double>(decision_count_.count());
}

#endif
