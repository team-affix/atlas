// rp_fewer_candidate_goal_candidates_activator: activate then set leaf score from -count.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/rp_fewer_candidate_goal_candidates_activator.hpp"
#include "value_objects/lineage.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::StrictMock;

namespace {

struct MockActivateGoalCandidates {
    MOCK_METHOD(bool, activate_goal_candidates, (const goal_lineage*));
};

struct MockComputeActiveGoalValue {
    MOCK_METHOD(double, compute_active_goal_value, (const goal_lineage*));
};

struct MockSetActiveGoalValue {
    MOCK_METHOD(void, set_active_goal_value, (const goal_lineage*, double));
};

using activator_t = rp_fewer_candidate_goal_candidates_activator<
    MockActivateGoalCandidates, MockComputeActiveGoalValue, MockSetActiveGoalValue>;

}  // namespace

struct RpFewerCandidateGoalCandidatesActivatorTest : public ::testing::Test {
    NiceMock<MockActivateGoalCandidates> activate;
    NiceMock<MockComputeActiveGoalValue> compute;
    StrictMock<MockSetActiveGoalValue> set_value;
    activator_t activator{activate, compute, set_value};
    goal_lineage gl{nullptr, 0};
};

TEST_F(RpFewerCandidateGoalCandidatesActivatorTest, SetsScoreAfterSuccessfulActivate) {
    EXPECT_CALL(activate, activate_goal_candidates(&gl)).WillOnce(Return(true));
    EXPECT_CALL(compute, compute_active_goal_value(&gl)).WillOnce(Return(-3.0));
    EXPECT_CALL(set_value, set_active_goal_value(&gl, -3.0));
    EXPECT_TRUE(activator.activate_goal_candidates(&gl));
}

TEST_F(RpFewerCandidateGoalCandidatesActivatorTest, SkipsSetWhenActivateFails) {
    EXPECT_CALL(activate, activate_goal_candidates(&gl)).WillOnce(Return(false));
    EXPECT_CALL(set_value, set_active_goal_value).Times(0);
    EXPECT_FALSE(activator.activate_goal_candidates(&gl));
}
