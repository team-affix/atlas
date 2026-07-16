#ifndef RIDGE_TEAR_DOWN_SIM_HPP
#define RIDGE_TEAR_DOWN_SIM_HPP

template<typename IComputeMctsReward, typename ISetValueDelta, typename ITearDownMcts, typename ITearDownSim>
struct ridge_tear_down_sim {
    ridge_tear_down_sim(IComputeMctsReward&, ISetValueDelta&, ITearDownMcts&, ITearDownSim&);
    void tear_down();
private:
    IComputeMctsReward& compute_mcts_reward_;
    ISetValueDelta& set_value_delta_;
    ITearDownMcts& tear_down_mcts_;
    ITearDownSim& tear_down_;
};

template<typename ICMR, typename ISVD, typename ITDM, typename ITDS>
ridge_tear_down_sim<ICMR, ISVD, ITDM, ITDS>::ridge_tear_down_sim(
    ICMR& compute_mcts_reward, ISVD& set_value_delta, ITDM& tear_down_mcts, ITDS& tear_down)
    : compute_mcts_reward_(compute_mcts_reward)
    , set_value_delta_(set_value_delta)
    , tear_down_mcts_(tear_down_mcts)
    , tear_down_(tear_down) {}

template<typename ICMR, typename ISVD, typename ITDM, typename ITDS>
void ridge_tear_down_sim<ICMR, ISVD, ITDM, ITDS>::tear_down() {
    set_value_delta_.set_value(compute_mcts_reward_.compute_mcts_reward());
    tear_down_mcts_.tear_down();
    tear_down_.tear_down();
}

#endif
