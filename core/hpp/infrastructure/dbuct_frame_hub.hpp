#ifndef DBUCT_FRAME_HUB_HPP
#define DBUCT_FRAME_HUB_HPP

#include <cstddef>
#include "infrastructure/coroutine.hpp"
#include "value_objects/lineage.hpp"

template<typename IFrameDepthTracker,
         typename IGoalExprs,
         typename IGoalCandidateRules,
         typename IChosenGoalCandidates,
         typename IDecisionMemory,
         typename IResolutionMemory,
         typename IUnitGoals,
         typename ICandidateFrameOffsets,
         typename IFrameBumpAllocator,
         typename INearestDecision,
         typename IEliminationBacklog,
         typename IAvoidanceUnitBoundary,
         typename ISrtActiveGoals,
         typename IBindMap,
         typename IMhu,
         typename ICdcl>
struct dbuct_frame_hub {
    dbuct_frame_hub(
        IFrameDepthTracker& depth_tracker,
        IGoalExprs& goal_exprs,
        IGoalCandidateRules& goal_candidate_rules,
        IChosenGoalCandidates& chosen_goal_candidates,
        IDecisionMemory& decision_memory,
        IResolutionMemory& resolution_memory,
        IUnitGoals& unit_goals,
        ICandidateFrameOffsets& candidate_frame_offsets,
        IFrameBumpAllocator& frame_bump_allocator,
        INearestDecision& nearest_decision,
        IEliminationBacklog& elimination_backlog,
        IAvoidanceUnitBoundary& avoidance_unit_boundary,
        ISrtActiveGoals& srt_active_goals,
        IBindMap& bind_map,
        IMhu& mhu,
        ICdcl& cdcl)
        : depth_tracker_(depth_tracker),
          goal_exprs_(goal_exprs),
          goal_candidate_rules_(goal_candidate_rules),
          chosen_goal_candidates_(chosen_goal_candidates),
          decision_memory_(decision_memory),
          resolution_memory_(resolution_memory),
          unit_goals_(unit_goals),
          candidate_frame_offsets_(candidate_frame_offsets),
          frame_bump_allocator_(frame_bump_allocator),
          nearest_decision_(nearest_decision),
          elimination_backlog_(elimination_backlog),
          avoidance_unit_boundary_(avoidance_unit_boundary),
          srt_active_goals_(srt_active_goals),
          bind_map_(bind_map),
          mhu_(mhu),
          cdcl_(cdcl) {}

    void push_frame();
    coroutine<const resolution_lineage*, void> pop_frame();
    size_t depth() const;

private:
    IFrameDepthTracker& depth_tracker_;
    IGoalExprs& goal_exprs_;
    IGoalCandidateRules& goal_candidate_rules_;
    IChosenGoalCandidates& chosen_goal_candidates_;
    IDecisionMemory& decision_memory_;
    IResolutionMemory& resolution_memory_;
    IUnitGoals& unit_goals_;
    ICandidateFrameOffsets& candidate_frame_offsets_;
    IFrameBumpAllocator& frame_bump_allocator_;
    INearestDecision& nearest_decision_;
    IEliminationBacklog& elimination_backlog_;
    IAvoidanceUnitBoundary& avoidance_unit_boundary_;
    ISrtActiveGoals& srt_active_goals_;
    IBindMap& bind_map_;
    IMhu& mhu_;
    ICdcl& cdcl_;
};

template<typename IFDT, typename IGE, typename IGCR, typename ICGC, typename IDM,
         typename IRM, typename IUG, typename ICFO, typename IFBA, typename IND,
         typename IEB, typename IAUB, typename ISAG, typename IBM, typename IMHU,
         typename ICDCL>
void dbuct_frame_hub<IFDT, IGE, IGCR, ICGC, IDM, IRM, IUG, ICFO, IFBA, IND,
                     IEB, IAUB, ISAG, IBM, IMHU, ICDCL>::push_frame() {
    depth_tracker_.push();
    goal_exprs_.push_frame();
    goal_candidate_rules_.push_frame();
    chosen_goal_candidates_.push_frame();
    decision_memory_.push_frame();
    resolution_memory_.push_frame();
    unit_goals_.push_frame();
    candidate_frame_offsets_.push_frame();
    frame_bump_allocator_.push_frame();
    nearest_decision_.push_frame();
    elimination_backlog_.push_frame();
    avoidance_unit_boundary_.push_frame();
    srt_active_goals_.push_frame();
    bind_map_.push_frame();
    mhu_.push_frame();
    cdcl_.push_frame();
}

template<typename IFDT, typename IGE, typename IGCR, typename ICGC, typename IDM,
         typename IRM, typename IUG, typename ICFO, typename IFBA, typename IND,
         typename IEB, typename IAUB, typename ISAG, typename IBM, typename IMHU,
         typename ICDCL>
coroutine<const resolution_lineage*, void>
dbuct_frame_hub<IFDT, IGE, IGCR, ICGC, IDM, IRM, IUG, ICFO, IFBA, IND,
                IEB, IAUB, ISAG, IBM, IMHU, ICDCL>::pop_frame() {
    mhu_.pop_frame();
    bind_map_.pop_frame();
    srt_active_goals_.pop_frame();
    avoidance_unit_boundary_.pop_frame();
    elimination_backlog_.pop_frame();
    nearest_decision_.pop_frame();
    frame_bump_allocator_.pop_frame();
    candidate_frame_offsets_.pop_frame();
    unit_goals_.pop_frame();
    resolution_memory_.pop_frame();
    decision_memory_.pop_frame();
    chosen_goal_candidates_.pop_frame();
    goal_candidate_rules_.pop_frame();
    goal_exprs_.pop_frame();
    depth_tracker_.pop();

    auto sm = cdcl_.pop_frame();
    while (!sm.done()) {
        sm.resume();
        if (sm.has_yield())
            co_yield sm.consume_yield();
    }
}

template<typename IFDT, typename IGE, typename IGCR, typename ICGC, typename IDM,
         typename IRM, typename IUG, typename ICFO, typename IFBA, typename IND,
         typename IEB, typename IAUB, typename ISAG, typename IBM, typename IMHU,
         typename ICDCL>
size_t dbuct_frame_hub<IFDT, IGE, IGCR, ICGC, IDM, IRM, IUG, ICFO, IFBA, IND,
                       IEB, IAUB, ISAG, IBM, IMHU, ICDCL>::depth() const {
    return depth_tracker_.depth();
}

#endif
