#ifndef RIDGE_TEAR_DOWN_SIM_HPP
#define RIDGE_TEAR_DOWN_SIM_HPP

template<typename IComputeMctsReward, typename ISetValueDelta, typename ITerminate, typename ITearDownSim>
struct ridge_tear_down_sim {
    ridge_tear_down_sim(IComputeMctsReward&, ISetValueDelta&, ITerminate&, ITearDownSim&);
    void tear_down();
private:
    IComputeMctsReward& compute_mcts_reward_;
    ISetValueDelta& set_value_delta_;
    ITerminate& terminate_;
    ITearDownSim& tear_down_;
};

template<typename ICMR, typename ISVD, typename IT, typename ITDS>
ridge_tear_down_sim<ICMR, ISVD, IT, ITDS>::ridge_tear_down_sim(
    ICMR& compute_mcts_reward, ISVD& set_value_delta, IT& terminate, ITDS& tear_down)
    : compute_mcts_reward_(compute_mcts_reward)
    , set_value_delta_(set_value_delta)
    , terminate_(terminate)
    , tear_down_(tear_down) {}

template<typename ICMR, typename ISVD, typename IT, typename ITDS>
void ridge_tear_down_sim<ICMR, ISVD, IT, ITDS>::tear_down() {
    set_value_delta_.set_value(compute_mcts_reward_.compute_mcts_reward());
    terminate_.terminate();
    tear_down_.tear_down();
}

#endif
