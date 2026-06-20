#ifndef HORIZON_REWARD_HPP
#define HORIZON_REWARD_HPP

template<typename IGetGroundedWeight>
struct horizon_reward {
    horizon_reward(IGetGroundedWeight& cgw);
    double compute_mcts_reward() const;
private:
    IGetGroundedWeight& grounded_weight_;
};

template<typename IGetGroundedWeight>
horizon_reward<IGetGroundedWeight>::horizon_reward(IGetGroundedWeight& cgw)
    : grounded_weight_(cgw) {}

template<typename IGetGroundedWeight>
double horizon_reward<IGetGroundedWeight>::compute_mcts_reward() const {
    return grounded_weight_.get();
}

#endif
