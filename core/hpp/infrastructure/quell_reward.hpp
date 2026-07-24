#ifndef QUELL_REWARD_HPP
#define QUELL_REWARD_HPP

template<typename IGetRemainingWork>
struct quell_reward {
    quell_reward(IGetRemainingWork& remaining_work);
    double compute_mcts_reward() const;
private:
    IGetRemainingWork& remaining_work_;
};

template<typename IGetRemainingWork>
quell_reward<IGetRemainingWork>::quell_reward(IGetRemainingWork& remaining_work)
    : remaining_work_(remaining_work) {}

template<typename IGetRemainingWork>
double quell_reward<IGetRemainingWork>::compute_mcts_reward() const {
    return -remaining_work_.get();
}

#endif
