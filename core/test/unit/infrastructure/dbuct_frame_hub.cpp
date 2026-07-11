// dbuct_frame_hub coordinates per-component frame stacks and optional MHU hooks.

#include <gtest/gtest.h>
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
#include "infrastructure/frame_depth_tracker.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/ra_rule_id_set_factory.hpp"
#include "value_objects/lineage.hpp"

namespace {

using bind_map_t = dbuct_bind_map<globalizer>;
using boundary_t = dbuct_avoidance_unit_boundary<dbuct_nearest_decision, frame_depth_tracker>;
using hub_t = dbuct_frame_hub<bind_map_t, boundary_t>;

struct hub_fixture {
    frame_depth_tracker depth_tracker;
    globalizer g;
    bind_map_t bind_map{g};
    ra_rule_id_set_factory rule_factory;
    dbuct_goal_exprs goal_exprs;
    dbuct_goal_candidate_rules goal_candidate_rules{rule_factory};
    dbuct_chosen_goal_candidates chosen_goal_candidates;
    dbuct_decision_memory decision_memory;
    dbuct_resolution_memory resolution_memory;
    dbuct_unit_goals unit_goals;
    dbuct_candidate_frame_offsets candidate_frame_offsets;
    dbuct_frame_bump_allocator frame_bump_allocator{0};
    dbuct_nearest_decision nearest_decision;
    dbuct_elimination_backlog elimination_backlog;
    boundary_t avoidance_unit_boundary{nearest_decision, depth_tracker};
    dbuct_srt_active_goals srt_active_goals;
    hub_t frame_hub;

    int mhu_push = 0;
    int mhu_pop = 0;

    hub_fixture()
        : frame_hub(depth_tracker,
                    goal_exprs,
                    goal_candidate_rules,
                    chosen_goal_candidates,
                    decision_memory,
                    resolution_memory,
                    unit_goals,
                    candidate_frame_offsets,
                    frame_bump_allocator,
                    nearest_decision,
                    elimination_backlog,
                    avoidance_unit_boundary,
                    srt_active_goals,
                    bind_map) {
        frame_hub.bind_mhu(
            [this]() { ++mhu_push; },
            [this]() { ++mhu_pop; });
    }
};

}  // namespace

TEST(DbuctFrameHubTest, PushPopTracksDepthAndMhuHooks) {
    hub_fixture f;
    EXPECT_EQ(f.depth_tracker.depth(), 0u);

    f.frame_hub.push_frame();
    EXPECT_EQ(f.depth_tracker.depth(), 1u);
    EXPECT_EQ(f.mhu_push, 1);
    EXPECT_EQ(f.mhu_pop, 0);

    f.frame_hub.pop_frame();
    EXPECT_EQ(f.depth_tracker.depth(), 0u);
    EXPECT_EQ(f.mhu_pop, 1);
}

TEST(DbuctFrameHubTest, PopRevertsJournaledGoalExpr) {
    hub_fixture f;
    goal_lineage gl{nullptr, 0};
    framed_expr fe{{nullptr}, 2};

    f.frame_hub.push_frame();
    f.goal_exprs.set(&gl, fe);
    f.frame_hub.pop_frame();
    EXPECT_THROW(f.goal_exprs.get(&gl), std::out_of_range);
}
