#ifndef HORIZON_TEAR_DOWN_SIM_HPP
#define HORIZON_TEAR_DOWN_SIM_HPP

template<typename ITearDownSim, typename IGoalWeights, typename ICumulativeGroundedWeight>
struct horizon_tear_down_sim {
    horizon_tear_down_sim(ITearDownSim&, IGoalWeights&, ICumulativeGroundedWeight&);
    void tear_down();
private:
    ITearDownSim& base_;
    IGoalWeights& goal_weights_;
    ICumulativeGroundedWeight& cumulative_grounded_weight_;
};

template<typename ITDS, typename IGW, typename ICGW>
horizon_tear_down_sim<ITDS,IGW,ICGW>::horizon_tear_down_sim(ITDS& tds, IGW& gw, ICGW& cgw)
    : base_(tds), goal_weights_(gw), cumulative_grounded_weight_(cgw) {}

template<typename ITDS, typename IGW, typename ICGW>
void horizon_tear_down_sim<ITDS,IGW,ICGW>::tear_down() {
    goal_weights_.clear_goal_weights();
    cumulative_grounded_weight_.clear();
    base_.tear_down();
}

#endif
