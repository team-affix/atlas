// goal_activator fetches the candidate frame offset, wraps the body subgoal
// as a framed_expr, and registers it via set_goal_expr.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/goal_activator.hpp"

using ::testing::_;
using ::testing::Return;

struct MockSetGoalExpr {
    MOCK_METHOD(void, set, (const goal_lineage*, framed_expr));
};

struct MockInsertGoalCandidates {
    MOCK_METHOD(void, insert, (const goal_lineage*));
};

struct MockInsertActiveGoal {
    MOCK_METHOD(void, insert_active_goal, (const goal_lineage*));
};

struct MockGetCandidateFrameOffset {
    MOCK_METHOD(uint32_t, get, (const resolution_lineage*), (const));
};

struct MockGetResolutionRule {
    MOCK_METHOD(const rule*, get, (const resolution_lineage*), (const));
};

using TestGoalActivator = goal_activator<
    MockSetGoalExpr, MockInsertGoalCandidates, MockInsertActiveGoal,
    MockGetCandidateFrameOffset, MockGetResolutionRule>;

struct GoalActivatorTest : public ::testing::Test {
    static constexpr rule_id kRule = 0;
    static constexpr subgoal_id kBodyIdx = 0;
    static constexpr uint32_t kFrameOffset = 42;

    MockSetGoalExpr set_goal_expr;
    MockInsertGoalCandidates insert_goal_candidates;
    MockInsertActiveGoal insert_active_goal;
    MockGetCandidateFrameOffset get_candidate_frame_offset;
    MockGetResolutionRule get_resolution_rule;
    TestGoalActivator activator{set_goal_expr, insert_goal_candidates, insert_active_goal,
                                get_candidate_frame_offset, get_resolution_rule};

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
