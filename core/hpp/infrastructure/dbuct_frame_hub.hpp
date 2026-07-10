#ifndef DBUCT_FRAME_HUB_HPP
#define DBUCT_FRAME_HUB_HPP

#include <cstddef>
#include <functional>
#include <optional>
#include "infrastructure/frame_depth_tracker.hpp"

struct dbuct_goal_exprs;
struct dbuct_goal_candidate_rules;
struct dbuct_chosen_goal_candidates;
struct dbuct_decision_memory;
struct dbuct_resolution_memory;
struct dbuct_unit_goals;
struct dbuct_candidate_frame_offsets;
struct dbuct_frame_bump_allocator;
struct dbuct_nearest_decision;
struct dbuct_elimination_backlog;
struct dbuct_srt_active_goals;

template<typename IBindMap, typename IAvoidanceUnitBoundary>
struct dbuct_frame_hub {
    dbuct_frame_hub(
        frame_depth_tracker&,
        dbuct_goal_exprs&,
        dbuct_goal_candidate_rules&,
        dbuct_chosen_goal_candidates&,
        dbuct_decision_memory&,
        dbuct_resolution_memory&,
        dbuct_unit_goals&,
        dbuct_candidate_frame_offsets&,
        dbuct_frame_bump_allocator&,
        dbuct_nearest_decision&,
        dbuct_elimination_backlog&,
        IAvoidanceUnitBoundary&,
        dbuct_srt_active_goals&,
        IBindMap&);

    void bind_mhu(std::function<void()> push, std::function<void()> pop, std::function<void()> squash);

    void push_frame();
    void pop_frame();
    void squash_frame();
    size_t depth() const;

private:
    frame_depth_tracker& depth_tracker_;
    dbuct_goal_exprs& goal_exprs_;
    dbuct_goal_candidate_rules& goal_candidate_rules_;
    dbuct_chosen_goal_candidates& chosen_goal_candidates_;
    dbuct_decision_memory& decision_memory_;
    dbuct_resolution_memory& resolution_memory_;
    dbuct_unit_goals& unit_goals_;
    dbuct_candidate_frame_offsets& candidate_frame_offsets_;
    dbuct_frame_bump_allocator& frame_bump_allocator_;
    dbuct_nearest_decision& nearest_decision_;
    dbuct_elimination_backlog& elimination_backlog_;
    IAvoidanceUnitBoundary& avoidance_unit_boundary_;
    dbuct_srt_active_goals& srt_active_goals_;
    IBindMap& bind_map_;
    std::function<void()> mhu_push_;
    std::function<void()> mhu_pop_;
    std::function<void()> mhu_squash_;
};

#include "infrastructure/dbuct_frame_hub_impl.hpp"

#endif
