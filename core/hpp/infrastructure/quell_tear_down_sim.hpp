#ifndef QUELL_TEAR_DOWN_SIM_HPP
#define QUELL_TEAR_DOWN_SIM_HPP

template<typename IComputeMctsReward, typename ISetValueDelta, typename ITearDownMcts,
         typename IClearGoalDepths, typename IClearGoalWorkValues,
         typename IClearRemainingWork, typename ITearDownSim>
struct quell_tear_down_sim {
    quell_tear_down_sim(IComputeMctsReward&, ISetValueDelta&, ITearDownMcts&,
                        IClearGoalDepths&, IClearGoalWorkValues&,
                        IClearRemainingWork&, ITearDownSim&);
    void tear_down();
private:
    IComputeMctsReward& compute_mcts_reward_;
    ISetValueDelta& set_value_delta_;
    ITearDownMcts& tear_down_mcts_;
    IClearGoalDepths& clear_goal_depths_;
    IClearGoalWorkValues& clear_goal_work_values_;
    IClearRemainingWork& clear_remaining_work_;
    ITearDownSim& tear_down_;
};

template<typename ICMR, typename ISVD, typename ITDM, typename ICGD, typename ICGWV,
         typename ICRW, typename ITDS>
quell_tear_down_sim<ICMR, ISVD, ITDM, ICGD, ICGWV, ICRW, ITDS>::quell_tear_down_sim(
    ICMR& compute_mcts_reward, ISVD& set_value_delta, ITDM& tear_down_mcts,
    ICGD& clear_goal_depths, ICGWV& clear_goal_work_values,
    ICRW& clear_remaining_work, ITDS& tear_down)
    : compute_mcts_reward_(compute_mcts_reward)
    , set_value_delta_(set_value_delta)
    , tear_down_mcts_(tear_down_mcts)
    , clear_goal_depths_(clear_goal_depths)
    , clear_goal_work_values_(clear_goal_work_values)
    , clear_remaining_work_(clear_remaining_work)
    , tear_down_(tear_down) {}

template<typename ICMR, typename ISVD, typename ITDM, typename ICGD, typename ICGWV,
         typename ICRW, typename ITDS>
void quell_tear_down_sim<ICMR, ISVD, ITDM, ICGD, ICGWV, ICRW, ITDS>::tear_down() {
    set_value_delta_.set_value(compute_mcts_reward_.compute_mcts_reward());
    tear_down_mcts_.tear_down();
    clear_goal_depths_.clear_goal_depths();
    clear_goal_work_values_.clear_goal_work_values();
    clear_remaining_work_.clear();
    tear_down_.tear_down();
}

#endif
