#ifndef GENIUS_TEAR_DOWN_SIM_HPP
#define GENIUS_TEAR_DOWN_SIM_HPP

template<typename ITearDownMcts, typename IClearGoalWeights, typename IClearGroundedWeight,
         typename ITearDownSim>
struct genius_tear_down_sim {
    genius_tear_down_sim(ITearDownMcts&, IClearGoalWeights&, IClearGroundedWeight&, ITearDownSim&);
    void tear_down();
private:
    ITearDownMcts& tear_down_mcts_;
    IClearGoalWeights& goal_weights_;
    IClearGroundedWeight& cumulative_grounded_weight_;
    ITearDownSim& tear_down_;
};

template<typename ITDM, typename ICGW, typename ICGRW, typename ITDS>
genius_tear_down_sim<ITDM, ICGW, ICGRW, ITDS>::genius_tear_down_sim(
    ITDM& tear_down_mcts, ICGW& goal_weights, ICGRW& cumulative_grounded_weight, ITDS& tear_down)
    : tear_down_mcts_(tear_down_mcts)
    , goal_weights_(goal_weights)
    , cumulative_grounded_weight_(cumulative_grounded_weight)
    , tear_down_(tear_down) {}

template<typename ITDM, typename ICGW, typename ICGRW, typename ITDS>
void genius_tear_down_sim<ITDM, ICGW, ICGRW, ITDS>::tear_down() {
    tear_down_mcts_.tear_down();
    goal_weights_.clear_goal_weights();
    cumulative_grounded_weight_.clear();
    tear_down_.tear_down();
}

#endif
