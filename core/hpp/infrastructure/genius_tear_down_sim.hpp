#ifndef GENIUS_TEAR_DOWN_SIM_HPP
#define GENIUS_TEAR_DOWN_SIM_HPP

template<typename ITerminate, typename IClearGoalWeights, typename IClearGroundedWeight,
         typename ITearDownSim>
struct genius_tear_down_sim {
    genius_tear_down_sim(ITerminate&, IClearGoalWeights&, IClearGroundedWeight&, ITearDownSim&);
    void tear_down();
private:
    ITerminate& terminate_;
    IClearGoalWeights& goal_weights_;
    IClearGroundedWeight& cumulative_grounded_weight_;
    ITearDownSim& tear_down_;
};

template<typename IT, typename ICGW, typename ICGRW, typename ITDS>
genius_tear_down_sim<IT, ICGW, ICGRW, ITDS>::genius_tear_down_sim(
    IT& terminate, ICGW& goal_weights, ICGRW& cumulative_grounded_weight, ITDS& tear_down)
    : terminate_(terminate)
    , goal_weights_(goal_weights)
    , cumulative_grounded_weight_(cumulative_grounded_weight)
    , tear_down_(tear_down) {}

template<typename IT, typename ICGW, typename ICGRW, typename ITDS>
void genius_tear_down_sim<IT, ICGW, ICGRW, ITDS>::tear_down() {
    terminate_.terminate();
    goal_weights_.clear_goal_weights();
    cumulative_grounded_weight_.clear();
    tear_down_.tear_down();
}

#endif
