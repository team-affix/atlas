#ifndef DBUCT_HORIZON_FRAME_HUB_HPP
#define DBUCT_HORIZON_FRAME_HUB_HPP

#include "infrastructure/coroutine.hpp"
#include "value_objects/lineage.hpp"

template<typename IPushSolverFrame,
         typename IPopSolverFrame,
         typename IPushGoalWeightsFrame,
         typename IPopGoalWeightsFrame,
         typename IPushCumulativeGroundedWeightFrame,
         typename IPopCumulativeGroundedWeightFrame>
struct dbuct_horizon_frame_hub {
    dbuct_horizon_frame_hub(
        IPushSolverFrame& push_solver_frame,
        IPopSolverFrame& pop_solver_frame,
        IPushGoalWeightsFrame& push_goal_weights_frame,
        IPopGoalWeightsFrame& pop_goal_weights_frame,
        IPushCumulativeGroundedWeightFrame& push_cumulative_grounded_weight_frame,
        IPopCumulativeGroundedWeightFrame& pop_cumulative_grounded_weight_frame);

    void push_solver_frame();
    coroutine<const resolution_lineage*, void> pop_solver_frame();

private:
    IPushSolverFrame& push_solver_frame_;
    IPopSolverFrame& pop_solver_frame_;
    IPushGoalWeightsFrame& push_goal_weights_frame_;
    IPopGoalWeightsFrame& pop_goal_weights_frame_;
    IPushCumulativeGroundedWeightFrame& push_cumulative_grounded_weight_frame_;
    IPopCumulativeGroundedWeightFrame& pop_cumulative_grounded_weight_frame_;
};

template<typename IPSF, typename IPopSF,
         typename IPGWF, typename IPopGWF,
         typename IPCGWF, typename IPopCGWF>
dbuct_horizon_frame_hub<IPSF, IPopSF, IPGWF, IPopGWF, IPCGWF, IPopCGWF>::
dbuct_horizon_frame_hub(
    IPSF& push_solver_frame,
    IPopSF& pop_solver_frame,
    IPGWF& push_goal_weights_frame,
    IPopGWF& pop_goal_weights_frame,
    IPCGWF& push_cumulative_grounded_weight_frame,
    IPopCGWF& pop_cumulative_grounded_weight_frame)
    : push_solver_frame_(push_solver_frame)
    , pop_solver_frame_(pop_solver_frame)
    , push_goal_weights_frame_(push_goal_weights_frame)
    , pop_goal_weights_frame_(pop_goal_weights_frame)
    , push_cumulative_grounded_weight_frame_(push_cumulative_grounded_weight_frame)
    , pop_cumulative_grounded_weight_frame_(pop_cumulative_grounded_weight_frame) {}

template<typename IPSF, typename IPopSF,
         typename IPGWF, typename IPopGWF,
         typename IPCGWF, typename IPopCGWF>
void dbuct_horizon_frame_hub<IPSF, IPopSF, IPGWF, IPopGWF, IPCGWF, IPopCGWF>::
push_solver_frame() {
    push_solver_frame_.push_solver_frame();
    push_goal_weights_frame_.push_frame();
    push_cumulative_grounded_weight_frame_.push_frame();
}

template<typename IPSF, typename IPopSF,
         typename IPGWF, typename IPopGWF,
         typename IPCGWF, typename IPopCGWF>
coroutine<const resolution_lineage*, void>
dbuct_horizon_frame_hub<IPSF, IPopSF, IPGWF, IPopGWF, IPCGWF, IPopCGWF>::
pop_solver_frame() {
    pop_cumulative_grounded_weight_frame_.pop_frame();
    pop_goal_weights_frame_.pop_frame();
    auto sm = pop_solver_frame_.pop_solver_frame();
    while (!sm.done()) {
        sm.resume();
        if (sm.has_yield())
            co_yield sm.consume_yield();
    }
}

#endif
