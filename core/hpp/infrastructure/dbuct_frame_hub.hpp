#ifndef DBUCT_FRAME_HUB_HPP
#define DBUCT_FRAME_HUB_HPP

#include <cstddef>
#include "infrastructure/coroutine.hpp"
#include "value_objects/lineage.hpp"

template<typename IPushSolverFrameDepthFrame,
         typename IPopSolverFrameDepthFrame,
         typename IPushGoalExprFrame,
         typename IPopGoalExprFrame,
         typename IPushGoalCandidateRulesFrame,
         typename IPopGoalCandidateRulesFrame,
         typename IPushChosenGoalCandidatesFrame,
         typename IPopChosenGoalCandidatesFrame,
         typename IPushDecisionMemoryFrame,
         typename IPopDecisionMemoryFrame,
         typename IPushResolutionMemoryFrame,
         typename IPopResolutionMemoryFrame,
         typename IPushUnitGoalsFrame,
         typename IPopUnitGoalsFrame,
         typename IPushCandidateFrameOffsetsFrame,
         typename IPopCandidateFrameOffsetsFrame,
         typename IPushFrameBumpAllocatorFrame,
         typename IPopFrameBumpAllocatorFrame,
         typename IPushNearestDecisionFrame,
         typename IPopNearestDecisionFrame,
         typename IPushEliminationBacklogFrame,
         typename IPopEliminationBacklogFrame,
         typename IPushAvoidanceUnitBoundaryFrame,
         typename IPopAvoidanceUnitBoundaryFrame,
         typename IPushSrtActiveGoalsFrame,
         typename IPopSrtActiveGoalsFrame,
         typename IPushBindMapFrame,
         typename IPopBindMapFrame,
         typename IPushMhuFrame,
         typename IPopMhuFrame,
         typename IPushCdclFrame,
         typename IPopCdclFrame>
struct dbuct_frame_hub {
    dbuct_frame_hub(
        IPushSolverFrameDepthFrame& push_solver_frame_depth_frame,
        IPopSolverFrameDepthFrame& pop_solver_frame_depth_frame,
        IPushGoalExprFrame& push_goal_expr_frame,
        IPopGoalExprFrame& pop_goal_expr_frame,
        IPushGoalCandidateRulesFrame& push_goal_candidate_rules_frame,
        IPopGoalCandidateRulesFrame& pop_goal_candidate_rules_frame,
        IPushChosenGoalCandidatesFrame& push_chosen_goal_candidates_frame,
        IPopChosenGoalCandidatesFrame& pop_chosen_goal_candidates_frame,
        IPushDecisionMemoryFrame& push_decision_memory_frame,
        IPopDecisionMemoryFrame& pop_decision_memory_frame,
        IPushResolutionMemoryFrame& push_resolution_memory_frame,
        IPopResolutionMemoryFrame& pop_resolution_memory_frame,
        IPushUnitGoalsFrame& push_unit_goals_frame,
        IPopUnitGoalsFrame& pop_unit_goals_frame,
        IPushCandidateFrameOffsetsFrame& push_candidate_frame_offsets_frame,
        IPopCandidateFrameOffsetsFrame& pop_candidate_frame_offsets_frame,
        IPushFrameBumpAllocatorFrame& push_frame_bump_allocator_frame,
        IPopFrameBumpAllocatorFrame& pop_frame_bump_allocator_frame,
        IPushNearestDecisionFrame& push_nearest_decision_frame,
        IPopNearestDecisionFrame& pop_nearest_decision_frame,
        IPushEliminationBacklogFrame& push_elimination_backlog_frame,
        IPopEliminationBacklogFrame& pop_elimination_backlog_frame,
        IPushAvoidanceUnitBoundaryFrame& push_avoidance_unit_boundary_frame,
        IPopAvoidanceUnitBoundaryFrame& pop_avoidance_unit_boundary_frame,
        IPushSrtActiveGoalsFrame& push_srt_active_goals_frame,
        IPopSrtActiveGoalsFrame& pop_srt_active_goals_frame,
        IPushBindMapFrame& push_bind_map_frame,
        IPopBindMapFrame& pop_bind_map_frame,
        IPushMhuFrame& push_mhu_frame,
        IPopMhuFrame& pop_mhu_frame,
        IPushCdclFrame& push_cdcl_frame,
        IPopCdclFrame& pop_cdcl_frame);

    void push_solver_frame();
    coroutine<const resolution_lineage*, void> pop_solver_frame();

private:
    IPushSolverFrameDepthFrame& push_solver_frame_depth_frame_;
    IPopSolverFrameDepthFrame& pop_solver_frame_depth_frame_;
    IPushGoalExprFrame& push_goal_expr_frame_;
    IPopGoalExprFrame& pop_goal_expr_frame_;
    IPushGoalCandidateRulesFrame& push_goal_candidate_rules_frame_;
    IPopGoalCandidateRulesFrame& pop_goal_candidate_rules_frame_;
    IPushChosenGoalCandidatesFrame& push_chosen_goal_candidates_frame_;
    IPopChosenGoalCandidatesFrame& pop_chosen_goal_candidates_frame_;
    IPushDecisionMemoryFrame& push_decision_memory_frame_;
    IPopDecisionMemoryFrame& pop_decision_memory_frame_;
    IPushResolutionMemoryFrame& push_resolution_memory_frame_;
    IPopResolutionMemoryFrame& pop_resolution_memory_frame_;
    IPushUnitGoalsFrame& push_unit_goals_frame_;
    IPopUnitGoalsFrame& pop_unit_goals_frame_;
    IPushCandidateFrameOffsetsFrame& push_candidate_frame_offsets_frame_;
    IPopCandidateFrameOffsetsFrame& pop_candidate_frame_offsets_frame_;
    IPushFrameBumpAllocatorFrame& push_frame_bump_allocator_frame_;
    IPopFrameBumpAllocatorFrame& pop_frame_bump_allocator_frame_;
    IPushNearestDecisionFrame& push_nearest_decision_frame_;
    IPopNearestDecisionFrame& pop_nearest_decision_frame_;
    IPushEliminationBacklogFrame& push_elimination_backlog_frame_;
    IPopEliminationBacklogFrame& pop_elimination_backlog_frame_;
    IPushAvoidanceUnitBoundaryFrame& push_avoidance_unit_boundary_frame_;
    IPopAvoidanceUnitBoundaryFrame& pop_avoidance_unit_boundary_frame_;
    IPushSrtActiveGoalsFrame& push_srt_active_goals_frame_;
    IPopSrtActiveGoalsFrame& pop_srt_active_goals_frame_;
    IPushBindMapFrame& push_bind_map_frame_;
    IPopBindMapFrame& pop_bind_map_frame_;
    IPushMhuFrame& push_mhu_frame_;
    IPopMhuFrame& pop_mhu_frame_;
    IPushCdclFrame& push_cdcl_frame_;
    IPopCdclFrame& pop_cdcl_frame_;
};

template<typename IPSFDF, typename IPopSFDF,
         typename IPGEF, typename IPopGEF,
         typename IPGCRF, typename IPopGCRF,
         typename IPCGCF, typename IPopCGCF,
         typename IPDMF, typename IPopDMF,
         typename IPRMF, typename IPopRMF,
         typename IPUGF, typename IPopUGF,
         typename IPCFOF, typename IPopCFOF,
         typename IPFBAF, typename IPopFBAF,
         typename IPNDF, typename IPopNDF,
         typename IPEBF, typename IPopEBF,
         typename IPAUBF, typename IPopAUBF,
         typename IPSAGF, typename IPopSAGF,
         typename IPBMF, typename IPopBMF,
         typename IPMHUF, typename IPopMHUF,
         typename IPCDF, typename IPopCDF>
dbuct_frame_hub<IPSFDF, IPopSFDF,
                IPGEF, IPopGEF,
                IPGCRF, IPopGCRF,
                IPCGCF, IPopCGCF,
                IPDMF, IPopDMF,
                IPRMF, IPopRMF,
                IPUGF, IPopUGF,
                IPCFOF, IPopCFOF,
                IPFBAF, IPopFBAF,
                IPNDF, IPopNDF,
                IPEBF, IPopEBF,
                IPAUBF, IPopAUBF,
                IPSAGF, IPopSAGF,
                IPBMF, IPopBMF,
                IPMHUF, IPopMHUF,
                IPCDF, IPopCDF>::dbuct_frame_hub(
    IPSFDF& push_solver_frame_depth_frame,
    IPopSFDF& pop_solver_frame_depth_frame,
    IPGEF& push_goal_expr_frame,
    IPopGEF& pop_goal_expr_frame,
    IPGCRF& push_goal_candidate_rules_frame,
    IPopGCRF& pop_goal_candidate_rules_frame,
    IPCGCF& push_chosen_goal_candidates_frame,
    IPopCGCF& pop_chosen_goal_candidates_frame,
    IPDMF& push_decision_memory_frame,
    IPopDMF& pop_decision_memory_frame,
    IPRMF& push_resolution_memory_frame,
    IPopRMF& pop_resolution_memory_frame,
    IPUGF& push_unit_goals_frame,
    IPopUGF& pop_unit_goals_frame,
    IPCFOF& push_candidate_frame_offsets_frame,
    IPopCFOF& pop_candidate_frame_offsets_frame,
    IPFBAF& push_frame_bump_allocator_frame,
    IPopFBAF& pop_frame_bump_allocator_frame,
    IPNDF& push_nearest_decision_frame,
    IPopNDF& pop_nearest_decision_frame,
    IPEBF& push_elimination_backlog_frame,
    IPopEBF& pop_elimination_backlog_frame,
    IPAUBF& push_avoidance_unit_boundary_frame,
    IPopAUBF& pop_avoidance_unit_boundary_frame,
    IPSAGF& push_srt_active_goals_frame,
    IPopSAGF& pop_srt_active_goals_frame,
    IPBMF& push_bind_map_frame,
    IPopBMF& pop_bind_map_frame,
    IPMHUF& push_mhu_frame,
    IPopMHUF& pop_mhu_frame,
    IPCDF& push_cdcl_frame,
    IPopCDF& pop_cdcl_frame)
    : push_solver_frame_depth_frame_(push_solver_frame_depth_frame),
      pop_solver_frame_depth_frame_(pop_solver_frame_depth_frame),
      push_goal_expr_frame_(push_goal_expr_frame),
      pop_goal_expr_frame_(pop_goal_expr_frame),
      push_goal_candidate_rules_frame_(push_goal_candidate_rules_frame),
      pop_goal_candidate_rules_frame_(pop_goal_candidate_rules_frame),
      push_chosen_goal_candidates_frame_(push_chosen_goal_candidates_frame),
      pop_chosen_goal_candidates_frame_(pop_chosen_goal_candidates_frame),
      push_decision_memory_frame_(push_decision_memory_frame),
      pop_decision_memory_frame_(pop_decision_memory_frame),
      push_resolution_memory_frame_(push_resolution_memory_frame),
      pop_resolution_memory_frame_(pop_resolution_memory_frame),
      push_unit_goals_frame_(push_unit_goals_frame),
      pop_unit_goals_frame_(pop_unit_goals_frame),
      push_candidate_frame_offsets_frame_(push_candidate_frame_offsets_frame),
      pop_candidate_frame_offsets_frame_(pop_candidate_frame_offsets_frame),
      push_frame_bump_allocator_frame_(push_frame_bump_allocator_frame),
      pop_frame_bump_allocator_frame_(pop_frame_bump_allocator_frame),
      push_nearest_decision_frame_(push_nearest_decision_frame),
      pop_nearest_decision_frame_(pop_nearest_decision_frame),
      push_elimination_backlog_frame_(push_elimination_backlog_frame),
      pop_elimination_backlog_frame_(pop_elimination_backlog_frame),
      push_avoidance_unit_boundary_frame_(push_avoidance_unit_boundary_frame),
      pop_avoidance_unit_boundary_frame_(pop_avoidance_unit_boundary_frame),
      push_srt_active_goals_frame_(push_srt_active_goals_frame),
      pop_srt_active_goals_frame_(pop_srt_active_goals_frame),
      push_bind_map_frame_(push_bind_map_frame),
      pop_bind_map_frame_(pop_bind_map_frame),
      push_mhu_frame_(push_mhu_frame),
      pop_mhu_frame_(pop_mhu_frame),
      push_cdcl_frame_(push_cdcl_frame),
      pop_cdcl_frame_(pop_cdcl_frame) {}

template<typename IPSFDF, typename IPopSFDF,
         typename IPGEF, typename IPopGEF,
         typename IPGCRF, typename IPopGCRF,
         typename IPCGCF, typename IPopCGCF,
         typename IPDMF, typename IPopDMF,
         typename IPRMF, typename IPopRMF,
         typename IPUGF, typename IPopUGF,
         typename IPCFOF, typename IPopCFOF,
         typename IPFBAF, typename IPopFBAF,
         typename IPNDF, typename IPopNDF,
         typename IPEBF, typename IPopEBF,
         typename IPAUBF, typename IPopAUBF,
         typename IPSAGF, typename IPopSAGF,
         typename IPBMF, typename IPopBMF,
         typename IPMHUF, typename IPopMHUF,
         typename IPCDF, typename IPopCDF>
void dbuct_frame_hub<IPSFDF, IPopSFDF,
                     IPGEF, IPopGEF,
                     IPGCRF, IPopGCRF,
                     IPCGCF, IPopCGCF,
                     IPDMF, IPopDMF,
                     IPRMF, IPopRMF,
                     IPUGF, IPopUGF,
                     IPCFOF, IPopCFOF,
                     IPFBAF, IPopFBAF,
                     IPNDF, IPopNDF,
                     IPEBF, IPopEBF,
                     IPAUBF, IPopAUBF,
                     IPSAGF, IPopSAGF,
                     IPBMF, IPopBMF,
                     IPMHUF, IPopMHUF,
                     IPCDF, IPopCDF>::push_solver_frame() {
    push_solver_frame_depth_frame_.push_frame();
    push_goal_expr_frame_.push_frame();
    push_goal_candidate_rules_frame_.push_frame();
    push_chosen_goal_candidates_frame_.push_frame();
    push_decision_memory_frame_.push_frame();
    push_resolution_memory_frame_.push_frame();
    push_unit_goals_frame_.push_frame();
    push_candidate_frame_offsets_frame_.push_frame();
    push_frame_bump_allocator_frame_.push_frame();
    push_nearest_decision_frame_.push_frame();
    push_elimination_backlog_frame_.push_frame();
    push_avoidance_unit_boundary_frame_.push_frame();
    push_srt_active_goals_frame_.push_frame();
    push_bind_map_frame_.push_frame();
    push_mhu_frame_.push_frame();
    push_cdcl_frame_.push_frame();
}

template<typename IPSFDF, typename IPopSFDF,
         typename IPGEF, typename IPopGEF,
         typename IPGCRF, typename IPopGCRF,
         typename IPCGCF, typename IPopCGCF,
         typename IPDMF, typename IPopDMF,
         typename IPRMF, typename IPopRMF,
         typename IPUGF, typename IPopUGF,
         typename IPCFOF, typename IPopCFOF,
         typename IPFBAF, typename IPopFBAF,
         typename IPNDF, typename IPopNDF,
         typename IPEBF, typename IPopEBF,
         typename IPAUBF, typename IPopAUBF,
         typename IPSAGF, typename IPopSAGF,
         typename IPBMF, typename IPopBMF,
         typename IPMHUF, typename IPopMHUF,
         typename IPCDF, typename IPopCDF>
coroutine<const resolution_lineage*, void>
dbuct_frame_hub<IPSFDF, IPopSFDF,
                IPGEF, IPopGEF,
                IPGCRF, IPopGCRF,
                IPCGCF, IPopCGCF,
                IPDMF, IPopDMF,
                IPRMF, IPopRMF,
                IPUGF, IPopUGF,
                IPCFOF, IPopCFOF,
                IPFBAF, IPopFBAF,
                IPNDF, IPopNDF,
                IPEBF, IPopEBF,
                IPAUBF, IPopAUBF,
                IPSAGF, IPopSAGF,
                IPBMF, IPopBMF,
                IPMHUF, IPopMHUF,
                IPCDF, IPopCDF>::pop_solver_frame() {
    // Reverse of push except CDCL last: AUB (and peers) restore before CDCL
    // evaluates emit-vs-arm against ultimate MCTS depth.
    pop_mhu_frame_.pop_frame();
    pop_bind_map_frame_.pop_frame();
    pop_srt_active_goals_frame_.pop_frame();
    pop_avoidance_unit_boundary_frame_.pop_frame();
    pop_elimination_backlog_frame_.pop_frame();
    pop_nearest_decision_frame_.pop_frame();
    pop_frame_bump_allocator_frame_.pop_frame();
    pop_candidate_frame_offsets_frame_.pop_frame();
    pop_unit_goals_frame_.pop_frame();
    pop_resolution_memory_frame_.pop_frame();
    pop_decision_memory_frame_.pop_frame();
    pop_chosen_goal_candidates_frame_.pop_frame();
    pop_goal_candidate_rules_frame_.pop_frame();
    pop_goal_expr_frame_.pop_frame();
    pop_solver_frame_depth_frame_.pop_frame();

    auto sm = pop_cdcl_frame_.pop_frame();
    while (!sm.done()) {
        sm.resume();
        if (sm.has_yield())
            co_yield sm.consume_yield();
    }
}

#endif
