#ifndef HORIZON_TEAR_DOWN_SIM_HPP
#define HORIZON_TEAR_DOWN_SIM_HPP

template<typename IComputeMctsReward, typename ISetValueDelta, typename ITearDownMcts,
         typename IClearGoalWeights, typename IClearGroundedWeight, typename ITearDownSim>
struct horizon_tear_down_sim {
    horizon_tear_down_sim(IComputeMctsReward&, ISetValueDelta&, ITearDownMcts&,
                          IClearGoalWeights&, IClearGroundedWeight&, ITearDownSim&);
    void tear_down();
private:
    IComputeMctsReward& compute_mcts_reward_;
    ISetValueDelta& set_value_delta_;
    ITearDownMcts& tear_down_mcts_;
    IClearGoalWeights& goal_weights_;
    IClearGroundedWeight& cumulative_grounded_weight_;
    ITearDownSim& tear_down_;
};

template<typename ICMR, typename ISVD, typename ITDM, typename ICGW, typename ICGRW, typename ITDS>
horizon_tear_down_sim<ICMR, ISVD, ITDM, ICGW, ICGRW, ITDS>::horizon_tear_down_sim(
    ICMR& compute_mcts_reward, ISVD& set_value_delta, ITDM& tear_down_mcts,
    ICGW& goal_weights, ICGRW& cumulative_grounded_weight, ITDS& tear_down)
    : compute_mcts_reward_(compute_mcts_reward)
    , set_value_delta_(set_value_delta)
    , tear_down_mcts_(tear_down_mcts)
    , goal_weights_(goal_weights)
    , cumulative_grounded_weight_(cumulative_grounded_weight)
    , tear_down_(tear_down) {}

template<typename ICMR, typename ISVD, typename ITDM, typename ICGW, typename ICGRW, typename ITDS>
void horizon_tear_down_sim<ICMR, ISVD, ITDM, ICGW, ICGRW, ITDS>::tear_down() {
    set_value_delta_.set_value(compute_mcts_reward_.compute_mcts_reward());
    tear_down_mcts_.tear_down();
    goal_weights_.clear_goal_weights();
    cumulative_grounded_weight_.clear();
    tear_down_.tear_down();
}

#endif
