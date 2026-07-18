#ifndef DBUCT_RP_HEURISTIC_ROLLOUT_FRAME_HUB_HPP
#define DBUCT_RP_HEURISTIC_ROLLOUT_FRAME_HUB_HPP

#include "infrastructure/coroutine.hpp"
#include "value_objects/lineage.hpp"

template<typename IPushSolverFrame,
         typename IPopSolverFrame,
         typename IPushHeuristicMapFrame,
         typename IPopHeuristicMapFrame>
struct dbuct_rp_heuristic_rollout_frame_hub {
    dbuct_rp_heuristic_rollout_frame_hub(
        IPushSolverFrame& push_solver_frame,
        IPopSolverFrame& pop_solver_frame,
        IPushHeuristicMapFrame& push_heuristic_map_frame,
        IPopHeuristicMapFrame& pop_heuristic_map_frame);

    void push_solver_frame();
    coroutine<const resolution_lineage*, void> pop_solver_frame();

private:
    IPushSolverFrame& push_solver_frame_;
    IPopSolverFrame& pop_solver_frame_;
    IPushHeuristicMapFrame& push_heuristic_map_frame_;
    IPopHeuristicMapFrame& pop_heuristic_map_frame_;
};

template<typename IPSF, typename IPopSF, typename IPHMF, typename IPopHMF>
dbuct_rp_heuristic_rollout_frame_hub<IPSF, IPopSF, IPHMF, IPopHMF>::
dbuct_rp_heuristic_rollout_frame_hub(
    IPSF& push_solver_frame,
    IPopSF& pop_solver_frame,
    IPHMF& push_heuristic_map_frame,
    IPopHMF& pop_heuristic_map_frame)
    : push_solver_frame_(push_solver_frame)
    , pop_solver_frame_(pop_solver_frame)
    , push_heuristic_map_frame_(push_heuristic_map_frame)
    , pop_heuristic_map_frame_(pop_heuristic_map_frame) {}

template<typename IPSF, typename IPopSF, typename IPHMF, typename IPopHMF>
void dbuct_rp_heuristic_rollout_frame_hub<IPSF, IPopSF, IPHMF, IPopHMF>::push_solver_frame() {
    push_solver_frame_.push_solver_frame();
    push_heuristic_map_frame_.push_frame();
}

template<typename IPSF, typename IPopSF, typename IPHMF, typename IPopHMF>
coroutine<const resolution_lineage*, void>
dbuct_rp_heuristic_rollout_frame_hub<IPSF, IPopSF, IPHMF, IPopHMF>::pop_solver_frame() {
    pop_heuristic_map_frame_.pop_frame();
    auto sm = pop_solver_frame_.pop_solver_frame();
    while (!sm.done()) {
        sm.resume();
        if (sm.has_yield())
            co_yield sm.consume_yield();
    }
}

#endif
