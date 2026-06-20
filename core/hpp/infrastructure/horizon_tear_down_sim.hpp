#ifndef HORIZON_TEAR_DOWN_SIM_HPP
#define HORIZON_TEAR_DOWN_SIM_HPP

template<typename ITearDownSim, typename IClearGoalWeights, typename IClearGroundedWeight>
struct horizon_tear_down_sim {
    horizon_tear_down_sim(ITearDownSim&, IClearGoalWeights&, IClearGroundedWeight&);
    void tear_down();
private:
    ITearDownSim& base_;
    IClearGoalWeights& goal_weights_;
    IClearGroundedWeight& cumulative_grounded_weight_;
};

template<typename ITDS, typename ICGW, typename ICGRW>
horizon_tear_down_sim<ITDS,ICGW,ICGRW>::horizon_tear_down_sim(ITDS& tds, ICGW& gw, ICGRW& cgw)
    : base_(tds), goal_weights_(gw), cumulative_grounded_weight_(cgw) {}

template<typename ITDS, typename ICGW, typename ICGRW>
void horizon_tear_down_sim<ITDS,ICGW,ICGRW>::tear_down() {
    goal_weights_.clear_goal_weights();
    cumulative_grounded_weight_.clear();
    base_.tear_down();
}

#endif
