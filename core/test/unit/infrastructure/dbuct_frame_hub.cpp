// dbuct_frame_hub coordinates per-component frame stacks, including the MHU.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
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
#include "infrastructure/coroutine.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/ra_rule_id_set_factory.hpp"
#include "value_objects/lineage.hpp"

namespace {

struct fake_mcts_frame_depth {
    size_t depth_value;
    size_t mcts_frame_depth() const { return depth_value; }
};

using bind_map_t = dbuct_bind_map<globalizer>;
using boundary_t = dbuct_avoidance_unit_boundary<dbuct_nearest_decision, fake_mcts_frame_depth>;

struct fake_mhu {
    int pushes = 0;
    int pops = 0;
    void push_frame() { ++pushes; }
    void pop_frame() { ++pops; }
};

struct fake_cdcl {
    int pushes = 0;
    int pops = 0;
    void push_frame() { ++pushes; }
    coroutine<const resolution_lineage*, void> pop_frame() {
        ++pops;
        co_return;
    }
};

void drain(coroutine<const resolution_lineage*, void> sm) {
    while (!sm.done())
        sm.resume();
}

using hub_t = dbuct_frame_hub<
    dbuct_decision_memory,
    dbuct_goal_exprs, dbuct_goal_exprs,
    dbuct_goal_candidate_rules, dbuct_goal_candidate_rules,
    dbuct_chosen_goal_candidates, dbuct_chosen_goal_candidates,
    dbuct_decision_memory, dbuct_decision_memory,
    dbuct_resolution_memory, dbuct_resolution_memory,
    dbuct_unit_goals, dbuct_unit_goals,
    dbuct_candidate_frame_offsets, dbuct_candidate_frame_offsets,
    dbuct_frame_bump_allocator, dbuct_frame_bump_allocator,
    dbuct_nearest_decision, dbuct_nearest_decision,
    dbuct_elimination_backlog, dbuct_elimination_backlog,
    boundary_t, boundary_t,
    dbuct_srt_active_goals, dbuct_srt_active_goals,
    bind_map_t, bind_map_t,
    fake_mhu, fake_mhu,
    fake_cdcl, fake_cdcl>;

struct hub_fixture {
    fake_mcts_frame_depth mcts_frame_depth{1};
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
    boundary_t avoidance_unit_boundary{nearest_decision, mcts_frame_depth};
    dbuct_srt_active_goals srt_active_goals;
    fake_mhu mhu;
    fake_cdcl cdcl;
    hub_t frame_hub;

    hub_fixture()
        : frame_hub(decision_memory,
                    goal_exprs, goal_exprs,
                    goal_candidate_rules, goal_candidate_rules,
                    chosen_goal_candidates, chosen_goal_candidates,
                    decision_memory, decision_memory,
                    resolution_memory, resolution_memory,
                    unit_goals, unit_goals,
                    candidate_frame_offsets, candidate_frame_offsets,
                    frame_bump_allocator, frame_bump_allocator,
                    nearest_decision, nearest_decision,
                    elimination_backlog, elimination_backlog,
                    avoidance_unit_boundary, avoidance_unit_boundary,
                    srt_active_goals, srt_active_goals,
                    bind_map, bind_map,
                    mhu, mhu,
                    cdcl, cdcl) {}
};

}  // namespace

TEST(DbuctFrameHubTest, PushPopTracksMhuHooks) {
    hub_fixture f;
    EXPECT_EQ(f.frame_hub.solver_frame_depth(), 1u);

    f.frame_hub.push_solver_frame();
    EXPECT_EQ(f.mhu.pushes, 1);
    EXPECT_EQ(f.mhu.pops, 0);
    EXPECT_EQ(f.cdcl.pushes, 1);
    EXPECT_EQ(f.cdcl.pops, 0);

    drain(f.frame_hub.pop_solver_frame());
    EXPECT_EQ(f.mhu.pops, 1);
    EXPECT_EQ(f.cdcl.pops, 1);
}

TEST(DbuctFrameHubTest, PopRevertsJournaledGoalExpr) {
    hub_fixture f;
    goal_lineage gl{nullptr, 0};
    framed_expr fe{{nullptr}, 2};

    f.frame_hub.push_solver_frame();
    f.goal_exprs.set(&gl, fe);
    drain(f.frame_hub.pop_solver_frame());
    EXPECT_THROW(f.goal_exprs.get(&gl), std::out_of_range);
}

TEST(DbuctFrameHubTest, SolverFrameDepthTracksDecisionMemoryCount) {
    hub_fixture f;
    resolution_lineage gp{nullptr, 1};
    goal_lineage g{&gp, 0};
    resolution_lineage rl{&g, 0};

    f.decision_memory.record_decision(&rl);
    EXPECT_EQ(f.frame_hub.solver_frame_depth(), 2u);
}

namespace {

coroutine<const resolution_lineage*, void> empty_cdcl_pop() {
    co_return;
}

struct MockGetDecisionCount {
    MOCK_METHOD(size_t, count, (), (const));
};

struct MockPushFrame {
    MOCK_METHOD(void, push_frame, ());
};

struct MockPopFrame {
    MOCK_METHOD(void, pop_frame, ());
};

struct MockPopCdclFrame {
    MOCK_METHOD((coroutine<const resolution_lineage*, void>), pop_frame, ());
};

using mock_hub_t = dbuct_frame_hub<
    MockGetDecisionCount,
    MockPushFrame, MockPopFrame,
    MockPushFrame, MockPopFrame,
    MockPushFrame, MockPopFrame,
    MockPushFrame, MockPopFrame,
    MockPushFrame, MockPopFrame,
    MockPushFrame, MockPopFrame,
    MockPushFrame, MockPopFrame,
    MockPushFrame, MockPopFrame,
    MockPushFrame, MockPopFrame,
    MockPushFrame, MockPopFrame,
    MockPushFrame, MockPopFrame,
    MockPushFrame, MockPopFrame,
    MockPushFrame, MockPopFrame,
    MockPushFrame, MockPopFrame,
    MockPushFrame, MockPopCdclFrame>;

struct DbuctFrameHubMockTest : public ::testing::Test {
    ::testing::StrictMock<MockGetDecisionCount> get_decision_count;
    ::testing::StrictMock<MockPushFrame> push_goal_expr;
    ::testing::StrictMock<MockPopFrame> pop_goal_expr;
    ::testing::StrictMock<MockPushFrame> push_goal_candidate_rules;
    ::testing::StrictMock<MockPopFrame> pop_goal_candidate_rules;
    ::testing::StrictMock<MockPushFrame> push_chosen_goal_candidates;
    ::testing::StrictMock<MockPopFrame> pop_chosen_goal_candidates;
    ::testing::StrictMock<MockPushFrame> push_decision_memory;
    ::testing::StrictMock<MockPopFrame> pop_decision_memory;
    ::testing::StrictMock<MockPushFrame> push_resolution_memory;
    ::testing::StrictMock<MockPopFrame> pop_resolution_memory;
    ::testing::StrictMock<MockPushFrame> push_unit_goals;
    ::testing::StrictMock<MockPopFrame> pop_unit_goals;
    ::testing::StrictMock<MockPushFrame> push_candidate_frame_offsets;
    ::testing::StrictMock<MockPopFrame> pop_candidate_frame_offsets;
    ::testing::StrictMock<MockPushFrame> push_frame_bump_allocator;
    ::testing::StrictMock<MockPopFrame> pop_frame_bump_allocator;
    ::testing::StrictMock<MockPushFrame> push_nearest_decision;
    ::testing::StrictMock<MockPopFrame> pop_nearest_decision;
    ::testing::StrictMock<MockPushFrame> push_elimination_backlog;
    ::testing::StrictMock<MockPopFrame> pop_elimination_backlog;
    ::testing::StrictMock<MockPushFrame> push_avoidance_unit_boundary;
    ::testing::StrictMock<MockPopFrame> pop_avoidance_unit_boundary;
    ::testing::StrictMock<MockPushFrame> push_srt_active_goals;
    ::testing::StrictMock<MockPopFrame> pop_srt_active_goals;
    ::testing::StrictMock<MockPushFrame> push_bind_map;
    ::testing::StrictMock<MockPopFrame> pop_bind_map;
    ::testing::StrictMock<MockPushFrame> push_mhu;
    ::testing::StrictMock<MockPopFrame> pop_mhu;
    ::testing::StrictMock<MockPushFrame> push_cdcl;
    ::testing::StrictMock<MockPopCdclFrame> pop_cdcl;

    mock_hub_t hub{
        get_decision_count,
        push_goal_expr, pop_goal_expr,
        push_goal_candidate_rules, pop_goal_candidate_rules,
        push_chosen_goal_candidates, pop_chosen_goal_candidates,
        push_decision_memory, pop_decision_memory,
        push_resolution_memory, pop_resolution_memory,
        push_unit_goals, pop_unit_goals,
        push_candidate_frame_offsets, pop_candidate_frame_offsets,
        push_frame_bump_allocator, pop_frame_bump_allocator,
        push_nearest_decision, pop_nearest_decision,
        push_elimination_backlog, pop_elimination_backlog,
        push_avoidance_unit_boundary, pop_avoidance_unit_boundary,
        push_srt_active_goals, pop_srt_active_goals,
        push_bind_map, pop_bind_map,
        push_mhu, pop_mhu,
        push_cdcl, pop_cdcl};
};

}  // namespace

TEST_F(DbuctFrameHubMockTest, SolverFrameDepthMatchesDecisionCountCollaborator) {
    EXPECT_CALL(get_decision_count, count()).WillOnce(::testing::Return(3));
    EXPECT_EQ(hub.solver_frame_depth(), 4u);
}

TEST_F(DbuctFrameHubMockTest, PushThenPopUnwindsInReverseOrder) {
    {
        ::testing::InSequence seq;
        EXPECT_CALL(push_goal_expr, push_frame());
        EXPECT_CALL(push_goal_candidate_rules, push_frame());
        EXPECT_CALL(push_chosen_goal_candidates, push_frame());
        EXPECT_CALL(push_decision_memory, push_frame());
        EXPECT_CALL(push_resolution_memory, push_frame());
        EXPECT_CALL(push_unit_goals, push_frame());
        EXPECT_CALL(push_candidate_frame_offsets, push_frame());
        EXPECT_CALL(push_frame_bump_allocator, push_frame());
        EXPECT_CALL(push_nearest_decision, push_frame());
        EXPECT_CALL(push_elimination_backlog, push_frame());
        EXPECT_CALL(push_avoidance_unit_boundary, push_frame());
        EXPECT_CALL(push_srt_active_goals, push_frame());
        EXPECT_CALL(push_bind_map, push_frame());
        EXPECT_CALL(push_mhu, push_frame());
        EXPECT_CALL(push_cdcl, push_frame());
    }
    hub.push_solver_frame();

    {
        ::testing::InSequence seq;
        EXPECT_CALL(pop_cdcl, pop_frame())
            .WillOnce(::testing::Return(::testing::ByMove(empty_cdcl_pop())));
        EXPECT_CALL(pop_mhu, pop_frame());
        EXPECT_CALL(pop_bind_map, pop_frame());
        EXPECT_CALL(pop_srt_active_goals, pop_frame());
        EXPECT_CALL(pop_avoidance_unit_boundary, pop_frame());
        EXPECT_CALL(pop_elimination_backlog, pop_frame());
        EXPECT_CALL(pop_nearest_decision, pop_frame());
        EXPECT_CALL(pop_frame_bump_allocator, pop_frame());
        EXPECT_CALL(pop_candidate_frame_offsets, pop_frame());
        EXPECT_CALL(pop_unit_goals, pop_frame());
        EXPECT_CALL(pop_resolution_memory, pop_frame());
        EXPECT_CALL(pop_decision_memory, pop_frame());
        EXPECT_CALL(pop_chosen_goal_candidates, pop_frame());
        EXPECT_CALL(pop_goal_candidate_rules, pop_frame());
        EXPECT_CALL(pop_goal_expr, pop_frame());
    }
    drain(hub.pop_solver_frame());
}
