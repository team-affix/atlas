// rp_fewer_candidate_srt_subgoals_activator: activate base, then score body goals.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include "infrastructure/rp_fewer_candidate_srt_subgoals_activator.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

using ::testing::InSequence;
using ::testing::Return;
using ::testing::StrictMock;

namespace {

struct MockActivateSubgoalsAndCandidates {
    MOCK_METHOD(bool, activate_subgoals_and_candidates, (const resolution_lineage*));
};

struct MockGetRule {
    MOCK_METHOD(const rule*, get_rule, (rule_id), (const));
};

struct MockMakeGoalLineage {
    MOCK_METHOD(const goal_lineage*, make_goal_lineage,
                (const resolution_lineage*, subgoal_id));
};

struct MockComputeActiveGoalValue {
    MOCK_METHOD(double, compute_active_goal_value, (const goal_lineage*));
};

struct MockSetActiveGoalValue {
    MOCK_METHOD(void, set_active_goal_value, (const goal_lineage*, double));
};

using activator_t = rp_fewer_candidate_srt_subgoals_activator<
    MockActivateSubgoalsAndCandidates, MockGetRule, MockMakeGoalLineage,
    MockComputeActiveGoalValue, MockSetActiveGoalValue>;

}  // namespace

struct RpFewerCandidateSrtSubgoalsActivatorTest : public ::testing::Test {
    StrictMock<MockActivateSubgoalsAndCandidates> activate;
    StrictMock<MockGetRule> get_rule;
    StrictMock<MockMakeGoalLineage> make_lineage;
    StrictMock<MockComputeActiveGoalValue> compute;
    StrictMock<MockSetActiveGoalValue> set_value;
    activator_t activator{activate, get_rule, make_lineage, compute, set_value};

    goal_lineage parent{nullptr, 0};
    goal_lineage child0{nullptr, 1};
    goal_lineage child1{nullptr, 2};
    resolution_lineage rl{&parent, 7};
};

TEST_F(RpFewerCandidateSrtSubgoalsActivatorTest, ScoresEachBodyGoalAfterActivate) {
    rule r{nullptr, {nullptr, nullptr}};

    InSequence seq;
    EXPECT_CALL(activate, activate_subgoals_and_candidates(&rl)).WillOnce(Return(true));
    EXPECT_CALL(get_rule, get_rule(7)).WillOnce(Return(&r));
    EXPECT_CALL(make_lineage, make_goal_lineage(&rl, 0)).WillOnce(Return(&child0));
    EXPECT_CALL(compute, compute_active_goal_value(&child0)).WillOnce(Return(-2.0));
    EXPECT_CALL(set_value, set_active_goal_value(&child0, -2.0));
    EXPECT_CALL(make_lineage, make_goal_lineage(&rl, 1)).WillOnce(Return(&child1));
    EXPECT_CALL(compute, compute_active_goal_value(&child1)).WillOnce(Return(-5.0));
    EXPECT_CALL(set_value, set_active_goal_value(&child1, -5.0));

    EXPECT_TRUE(activator.activate_subgoals_and_candidates(&rl));
}

TEST_F(RpFewerCandidateSrtSubgoalsActivatorTest, EmptyBodySkipsSet) {
    rule r{nullptr, {}};

    InSequence seq;
    EXPECT_CALL(activate, activate_subgoals_and_candidates(&rl)).WillOnce(Return(true));
    EXPECT_CALL(get_rule, get_rule(7)).WillOnce(Return(&r));
    EXPECT_CALL(set_value, set_active_goal_value).Times(0);

    EXPECT_TRUE(activator.activate_subgoals_and_candidates(&rl));
}

TEST_F(RpFewerCandidateSrtSubgoalsActivatorTest, PropagatesInnerFalseWithoutScoring) {
    EXPECT_CALL(activate, activate_subgoals_and_candidates(&rl)).WillOnce(Return(false));
    EXPECT_CALL(get_rule, get_rule).Times(0);
    EXPECT_CALL(set_value, set_active_goal_value).Times(0);

    EXPECT_FALSE(activator.activate_subgoals_and_candidates(&rl));
}
