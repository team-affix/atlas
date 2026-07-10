#include "infrastructure/dbuct_avoidance_unit_boundary.hpp"
#include "infrastructure/dbuct_bind_map.hpp"
#include "infrastructure/dbuct_candidate_frame_offsets.hpp"
#include "infrastructure/dbuct_chosen_goal_candidates.hpp"
#include "infrastructure/dbuct_decision_memory.hpp"
#include "infrastructure/dbuct_elimination_backlog.hpp"
#include "infrastructure/dbuct_frame_bump_allocator.hpp"
#include "infrastructure/dbuct_frame_hub.hpp"
#include "infrastructure/dbuct_goal_candidate_rules.hpp"
#include "infrastructure/dbuct_goal_exprs.hpp"
#include "infrastructure/dbuct_nearest_decision.hpp"
#include "infrastructure/dbuct_resolution_memory.hpp"
#include "infrastructure/dbuct_srt_active_goals.hpp"
#include "infrastructure/dbuct_unit_goals.hpp"

template<typename IBindMap, typename IAvoidanceUnitBoundary>
dbuct_frame_hub<IBindMap, IAvoidanceUnitBoundary>::dbuct_frame_hub(
    frame_depth_tracker& depth_tracker,
    dbuct_goal_exprs& goal_exprs,
    dbuct_goal_candidate_rules& goal_candidate_rules,
    dbuct_chosen_goal_candidates& chosen_goal_candidates,
    dbuct_decision_memory& decision_memory,
    dbuct_resolution_memory& resolution_memory,
    dbuct_unit_goals& unit_goals,
    dbuct_candidate_frame_offsets& candidate_frame_offsets,
    dbuct_frame_bump_allocator& frame_bump_allocator,
    dbuct_nearest_decision& nearest_decision,
    dbuct_elimination_backlog& elimination_backlog,
    IAvoidanceUnitBoundary& avoidance_unit_boundary,
    dbuct_srt_active_goals& srt_active_goals,
    IBindMap& bind_map)
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
      bind_map_(bind_map) {}

template<typename IBindMap, typename IAvoidanceUnitBoundary>
void dbuct_frame_hub<IBindMap, IAvoidanceUnitBoundary>::bind_mhu(
    std::function<void()> push,
    std::function<void()> pop,
    std::function<void()> squash) {
    mhu_push_ = std::move(push);
    mhu_pop_ = std::move(pop);
    mhu_squash_ = std::move(squash);
}

template<typename IBindMap, typename IAvoidanceUnitBoundary>
void dbuct_frame_hub<IBindMap, IAvoidanceUnitBoundary>::push_frame() {
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
    if (mhu_push_)
        mhu_push_();
}

template<typename IBindMap, typename IAvoidanceUnitBoundary>
void dbuct_frame_hub<IBindMap, IAvoidanceUnitBoundary>::pop_frame() {
    if (mhu_pop_)
        mhu_pop_();
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
}

template<typename IBindMap, typename IAvoidanceUnitBoundary>
void dbuct_frame_hub<IBindMap, IAvoidanceUnitBoundary>::squash_frame() {
    if (mhu_squash_)
        mhu_squash_();
    bind_map_.squash_frame();
    srt_active_goals_.squash_frame();
    avoidance_unit_boundary_.squash_frame();
    elimination_backlog_.squash_frame();
    nearest_decision_.squash_frame();
    frame_bump_allocator_.squash_frame();
    candidate_frame_offsets_.squash_frame();
    unit_goals_.squash_frame();
    resolution_memory_.squash_frame();
    decision_memory_.squash_frame();
    chosen_goal_candidates_.squash_frame();
    goal_candidate_rules_.squash_frame();
    goal_exprs_.squash_frame();
    depth_tracker_.pop();
}

template<typename IBindMap, typename IAvoidanceUnitBoundary>
size_t dbuct_frame_hub<IBindMap, IAvoidanceUnitBoundary>::depth() const {
    return depth_tracker_.depth();
}
