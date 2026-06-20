#ifndef HORIZON_REWARD_HPP
#define HORIZON_REWARD_HPP

template<typename ICumulativeGroundedWeight>
struct horizon_reward {
    horizon_reward(ICumulativeGroundedWeight& cgw);
    double compute_mcts_reward() const;
private:
    ICumulativeGroundedWeight& grounded_weight_;
};

template<typename ICumulativeGroundedWeight>
horizon_reward<ICumulativeGroundedWeight>::horizon_reward(ICumulativeGroundedWeight& cgw)
    : grounded_weight_(cgw) {}

template<typename ICumulativeGroundedWeight>
double horizon_reward<ICumulativeGroundedWeight>::compute_mcts_reward() const {
    return grounded_weight_.get();
}

#endif
