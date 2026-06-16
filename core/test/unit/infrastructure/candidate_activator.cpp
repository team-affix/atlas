// candidate_activator: bumps frame allocator, constructs framed head, checks backlog
// and MHU acceptance, then stores frame offset and links goal-candidate.
// Backlogged or MHU-rejected cases must skip side effects.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "locator_fixture.hpp"
#include "infrastructure/candidate_activator.hpp"
#include "interfaces/i_frame_allocator.hpp"
#include "interfaces/i_set_candidate_frame_offset.hpp"
#include "interfaces/i_try_add_mhu_head.hpp"
#include "interfaces/i_is_backlogged_elimination.hpp"
#include "interfaces/i_get_goal_expr.hpp"
#include "interfaces/i_get_rule.hpp"
#include "interfaces/i_link_goal_candidate.hpp"
#include "functor_fixture.hpp"

using ::testing::_;
using ::testing::Return;

struct MockFrameAllocator : public i_frame_allocator {
    MOCK_METHOD(uint32_t, bump, (uint32_t n), (override));
    MOCK_METHOD(uint32_t, peek, (), (const, override));
    MOCK_METHOD(void, reset, (), (override));
};

struct MockSetCandidateFrameOffset : public i_set_candidate_frame_offset {
    MOCK_METHOD(void, set, (const resolution_lineage*, uint32_t frame_offset), (override));
};

struct MockTryAddMhuHead : public i_try_add_mhu_head {
    MOCK_METHOD(bool, try_add_head, (const resolution_lineage*, framed_expr, framed_expr), (override));
};

struct MockIsBackloggedElimination : public i_is_backlogged_elimination {
    MOCK_METHOD(bool, is_backlogged_elimination, (const resolution_lineage*), (const, override));
};

struct MockGetGoalExpr : public i_get_goal_expr {
    MOCK_METHOD(framed_expr, get, (const goal_lineage*), (const, override));
};

struct MockGetRule : public i_get_rule {
    MOCK_METHOD(const rule*, get, (rule_id), (const, override));
};

struct MockLinkGoalCandidate : public i_link_goal_candidate {
    MOCK_METHOD(void, link_goal_candidate, (const goal_lineage*, rule_id), (override));
};

struct CandidateActivatorTest : public ::testing::Test {
    test_functors functors;
    static constexpr rule_id kRule = 0;
    static constexpr subgoal_id kGoal = 0;
    static constexpr uint32_t kFrameOffset = 10;

    locator loc;
    MockFrameAllocator frame_alloc;
    MockSetCandidateFrameOffset set_frame;
    MockTryAddMhuHead mhu;
    MockIsBackloggedElimination is_backlogged;
    MockGetGoalExpr get_goal_expr;
    MockGetRule get_rule;
    MockLinkGoalCandidate link;
    candidate_activator activator;

    CandidateActivatorTest() : activator(init_activator()) {}

    candidate_activator init_activator() {
        loc.bind_as<i_frame_allocator>(frame_alloc);
        loc.bind_as<i_set_candidate_frame_offset>(set_frame);
        loc.bind_as<i_try_add_mhu_head>(mhu);
        loc.bind_as<i_is_backlogged_elimination>(is_backlogged);
        loc.bind_as<i_get_goal_expr>(get_goal_expr);
        loc.bind_as<i_get_rule>(get_rule);
        loc.bind_as<i_link_goal_candidate>(link);
        return candidate_activator{loc};
    }

    expr goal_e{expr::var{0}};
    expr head{expr::var{10}};
    expr body_subgoal{expr::var{1}};
    rule r{&head, {&body_subgoal}, 3 /* var_count */};
    goal_lineage parent{nullptr, kGoal};
    resolution_lineage rl{&parent, kRule};
};

TEST_F(CandidateActivatorTest, BackloggedSkipsAllSideEffects) {
    EXPECT_CALL(get_rule, get(kRule)).WillOnce(Return(&r));
    EXPECT_CALL(is_backlogged, is_backlogged_elimination(&rl)).WillOnce(Return(true));
    EXPECT_CALL(frame_alloc, bump).Times(0);
    EXPECT_CALL(get_goal_expr, get).Times(0);
    EXPECT_CALL(mhu, try_add_head).Times(0);
    EXPECT_CALL(set_frame, set).Times(0);
    EXPECT_CALL(link, link_goal_candidate).Times(0);
    activator.activate(&rl);
}

TEST_F(CandidateActivatorTest, RejectedHeadSkipsFrameStoreAndLink) {
    const framed_expr goal_fe{&goal_e, 0};
    EXPECT_CALL(get_rule, get(kRule)).WillOnce(Return(&r));
    EXPECT_CALL(is_backlogged, is_backlogged_elimination(&rl)).WillOnce(Return(false));
    EXPECT_CALL(frame_alloc, bump(3)).WillOnce(Return(kFrameOffset));
    EXPECT_CALL(get_goal_expr, get(&parent)).WillOnce(Return(goal_fe));
    EXPECT_CALL(mhu, try_add_head(&rl, goal_fe, framed_expr{&head, kFrameOffset}))
        .WillOnce(Return(false));
    EXPECT_CALL(set_frame, set).Times(0);
    EXPECT_CALL(link, link_goal_candidate).Times(0);
    activator.activate(&rl);
}

TEST_F(CandidateActivatorTest, AcceptedHeadStoresFrameOffsetAndLinks) {
    const framed_expr goal_fe{&goal_e, 0};
    EXPECT_CALL(get_rule, get(kRule)).WillOnce(Return(&r));
    EXPECT_CALL(is_backlogged, is_backlogged_elimination(&rl)).WillOnce(Return(false));
    EXPECT_CALL(frame_alloc, bump(3)).WillOnce(Return(kFrameOffset));
    EXPECT_CALL(get_goal_expr, get(&parent)).WillOnce(Return(goal_fe));
    EXPECT_CALL(mhu, try_add_head(&rl, goal_fe, framed_expr{&head, kFrameOffset}))
        .WillOnce(Return(true));
    EXPECT_CALL(set_frame, set(&rl, kFrameOffset)).Times(1);
    EXPECT_CALL(link, link_goal_candidate(&parent, kRule)).Times(1);
    activator.activate(&rl);
}
