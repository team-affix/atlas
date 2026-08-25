#ifndef HORIZON_TEAR_DOWN_SIM_HPP
#define HORIZON_TEAR_DOWN_SIM_HPP

template<typename IComputeMctsReward, typename ISetValueDelta, typename ITerminate,
         typename IClearGoalWeights, typename IClearGroundedWeight, typename ITearDownSim>
struct horizon_tear_down_sim {
    horizon_tear_down_sim(IComputeMctsReward&, ISetValueDelta&, ITerminate&,
                          IClearGoalWeights&, IClearGroundedWeight&, ITearDownSim&);
    void tear_down();
private:
    IComputeMctsReward& compute_mcts_reward_;
    ISetValueDelta& set_value_delta_;
    ITerminate& terminate_;
    IClearGoalWeights& goal_weights_;
    IClearGroundedWeight& cumulative_grounded_weight_;
    ITearDownSim& tear_down_;
};

template<typename ICMR, typename ISVD, typename IT, typename ICGW, typename ICGRW, typename ITDS>
horizon_tear_down_sim<ICMR, ISVD, IT, ICGW, ICGRW, ITDS>::horizon_tear_down_sim(
    ICMR& compute_mcts_reward, ISVD& set_value_delta, IT& terminate,
    ICGW& goal_weights, ICGRW& cumulative_grounded_weight, ITDS& tear_down)
    : compute_mcts_reward_(compute_mcts_reward)
    , set_value_delta_(set_value_delta)
    , terminate_(terminate)
    , goal_weights_(goal_weights)
    , cumulative_grounded_weight_(cumulative_grounded_weight)
    , tear_down_(tear_down) {}

template<typename ICMR, typename ISVD, typename IT, typename ICGW, typename ICGRW, typename ITDS>
void horizon_tear_down_sim<ICMR, ISVD, IT, ICGW, ICGRW, ITDS>::tear_down() {
    set_value_delta_.set_value(compute_mcts_reward_.compute_mcts_reward());
    terminate_.terminate();
    goal_weights_.clear_goal_weights();
    cumulative_grounded_weight_.clear();
    tear_down_.tear_down();
}

#endif
