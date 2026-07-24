#ifndef DBUCT_QUELL_FRAME_HUB_HPP
#define DBUCT_QUELL_FRAME_HUB_HPP

#include "infrastructure/coroutine.hpp"
#include "value_objects/lineage.hpp"

template<typename IPushSolverFrame,
         typename IPopSolverFrame,
         typename IPushGoalDepthsFrame,
         typename IPopGoalDepthsFrame,
         typename IPushGoalWorkValuesFrame,
         typename IPopGoalWorkValuesFrame,
         typename IPushRemainingWorkFrame,
         typename IPopRemainingWorkFrame>
struct dbuct_quell_frame_hub {
    dbuct_quell_frame_hub(
        IPushSolverFrame& push_solver_frame,
        IPopSolverFrame& pop_solver_frame,
        IPushGoalDepthsFrame& push_goal_depths_frame,
        IPopGoalDepthsFrame& pop_goal_depths_frame,
        IPushGoalWorkValuesFrame& push_goal_work_values_frame,
        IPopGoalWorkValuesFrame& pop_goal_work_values_frame,
        IPushRemainingWorkFrame& push_remaining_work_frame,
        IPopRemainingWorkFrame& pop_remaining_work_frame);

    void push_solver_frame();
    coroutine<const resolution_lineage*, void> pop_solver_frame();

private:
    IPushSolverFrame& push_solver_frame_;
    IPopSolverFrame& pop_solver_frame_;
    IPushGoalDepthsFrame& push_goal_depths_frame_;
    IPopGoalDepthsFrame& pop_goal_depths_frame_;
    IPushGoalWorkValuesFrame& push_goal_work_values_frame_;
    IPopGoalWorkValuesFrame& pop_goal_work_values_frame_;
    IPushRemainingWorkFrame& push_remaining_work_frame_;
    IPopRemainingWorkFrame& pop_remaining_work_frame_;
};

template<typename IPSF, typename IPopSF,
         typename IPGDF, typename IPopGDF,
         typename IPGWVF, typename IPopGWVF,
         typename IPRWF, typename IPopRWF>
dbuct_quell_frame_hub<IPSF, IPopSF, IPGDF, IPopGDF, IPGWVF, IPopGWVF, IPRWF, IPopRWF>::
dbuct_quell_frame_hub(
    IPSF& push_solver_frame,
    IPopSF& pop_solver_frame,
    IPGDF& push_goal_depths_frame,
    IPopGDF& pop_goal_depths_frame,
    IPGWVF& push_goal_work_values_frame,
    IPopGWVF& pop_goal_work_values_frame,
    IPRWF& push_remaining_work_frame,
    IPopRWF& pop_remaining_work_frame)
    : push_solver_frame_(push_solver_frame)
    , pop_solver_frame_(pop_solver_frame)
    , push_goal_depths_frame_(push_goal_depths_frame)
    , pop_goal_depths_frame_(pop_goal_depths_frame)
    , push_goal_work_values_frame_(push_goal_work_values_frame)
    , pop_goal_work_values_frame_(pop_goal_work_values_frame)
    , push_remaining_work_frame_(push_remaining_work_frame)
    , pop_remaining_work_frame_(pop_remaining_work_frame) {}

template<typename IPSF, typename IPopSF,
         typename IPGDF, typename IPopGDF,
         typename IPGWVF, typename IPopGWVF,
         typename IPRWF, typename IPopRWF>
void dbuct_quell_frame_hub<IPSF, IPopSF, IPGDF, IPopGDF, IPGWVF, IPopGWVF, IPRWF, IPopRWF>::
push_solver_frame() {
    push_solver_frame_.push_solver_frame();
    push_goal_depths_frame_.push_frame();
    push_goal_work_values_frame_.push_frame();
    push_remaining_work_frame_.push_frame();
}

template<typename IPSF, typename IPopSF,
         typename IPGDF, typename IPopGDF,
         typename IPGWVF, typename IPopGWVF,
         typename IPRWF, typename IPopRWF>
coroutine<const resolution_lineage*, void>
dbuct_quell_frame_hub<IPSF, IPopSF, IPGDF, IPopGDF, IPGWVF, IPopGWVF, IPRWF, IPopRWF>::
pop_solver_frame() {
    pop_remaining_work_frame_.pop_frame();
    pop_goal_work_values_frame_.pop_frame();
    pop_goal_depths_frame_.pop_frame();
    auto sm = pop_solver_frame_.pop_solver_frame();
    while (!sm.done()) {
        sm.resume();
        if (sm.has_yield())
            co_yield sm.consume_yield();
    }
}

#endif
