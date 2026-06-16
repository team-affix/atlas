// goal_activator fetches the candidate frame offset, wraps the body subgoal
// as a framed_expr, and registers it via i_set_goal_expr.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "locator_fixture.hpp"
#include "infrastructure/goal_activator.hpp"
#include "interfaces/i_set_goal_expr.hpp"
#include "interfaces/i_insert_goal_candidates.hpp"
#include "interfaces/i_insert_active_goal.hpp"
#include "interfaces/i_get_candidate_frame_offset.hpp"
#include "interfaces/i_get_resolution_rule.hpp"

using ::testing::_;
using ::testing::Return;

struct MockSetGoalExpr : public i_set_goal_expr {
    MOCK_METHOD(void, set, (const goal_lineage*, framed_expr), (override));
};

struct MockInsertGoalCandidates : public i_insert_goal_candidates {
    MOCK_METHOD(void, insert, (const goal_lineage*), (override));
};

struct MockInsertActiveGoal : public i_insert_active_goal {
    MOCK_METHOD(void, insert_active_goal, (const goal_lineage*), (override));
};

struct MockGetCandidateFrameOffset : public i_get_candidate_frame_offset {
    MOCK_METHOD(uint32_t, get, (const resolution_lineage*), (const, override));
};

struct MockGetResolutionRule : public i_get_resolution_rule {
    MOCK_METHOD(const rule*, get, (const resolution_lineage*), (const, override));
};

struct GoalActivatorTest : public ::testing::Test {
    static constexpr rule_id kRule = 0;
    static constexpr subgoal_id kBodyIdx = 0;
    static constexpr uint32_t kFrameOffset = 42;

    locator loc;
    MockSetGoalExpr set_goal_expr;
    MockInsertGoalCandidates insert_goal_candidates;
    MockInsertActiveGoal insert_active_goal;
    MockGetCandidateFrameOffset get_candidate_frame_offset;
    MockGetResolutionRule get_resolution_rule;
    goal_activator activator;

    GoalActivatorTest() : activator(init_activator()) {}

    goal_activator init_activator() {
        loc.bind_as<i_set_goal_expr>(set_goal_expr);
        loc.bind_as<i_insert_goal_candidates>(insert_goal_candidates);
        loc.bind_as<i_insert_active_goal>(insert_active_goal);
        loc.bind_as<i_get_candidate_frame_offset>(get_candidate_frame_offset);
        loc.bind_as<i_get_resolution_rule>(get_resolution_rule);
        return goal_activator{loc};
    }

    expr child_goal{expr::var{1}};
    expr rule_head{expr::var{10}};
    rule parent_rule{&rule_head, {&child_goal}, 2};

    goal_lineage parent_gl{nullptr, 0};
    resolution_lineage res{&parent_gl, kRule};
    goal_lineage child_gl{&res, kBodyIdx};
};

TEST_F(GoalActivatorTest, ActivateWrapsBodySubgoalWithFrameOffset) {
    EXPECT_CALL(get_resolution_rule, get(&res)).WillOnce(Return(&parent_rule));
    EXPECT_CALL(get_candidate_frame_offset, get(&res)).WillOnce(Return(kFrameOffset));
    EXPECT_CALL(set_goal_expr, set(&child_gl, framed_expr{&child_goal, kFrameOffset})).Times(1);
    EXPECT_CALL(insert_goal_candidates, insert(&child_gl)).Times(1);
    EXPECT_CALL(insert_active_goal, insert_active_goal(&child_gl)).Times(1);
    activator.activate(&child_gl);
}

TEST_F(GoalActivatorTest, ActivateUsesBodyIndexForSubgoalExpr) {
    expr second_body{expr::var{2}};
    rule two_body{&rule_head, {&child_goal, &second_body}, 3};
    goal_lineage second_gl{&res, 1};

    EXPECT_CALL(get_resolution_rule, get(&res)).WillOnce(Return(&two_body));
    EXPECT_CALL(get_candidate_frame_offset, get(&res)).WillOnce(Return(kFrameOffset));
    EXPECT_CALL(set_goal_expr, set(&second_gl, framed_expr{&second_body, kFrameOffset})).Times(1);
    EXPECT_CALL(insert_goal_candidates, insert(&second_gl)).Times(1);
    EXPECT_CALL(insert_active_goal, insert_active_goal(&second_gl)).Times(1);
    activator.activate(&second_gl);
}
