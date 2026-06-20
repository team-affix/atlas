// horizon_goal_deactivator: erases goal weight, then delegates to srt_goal_deactivator.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/horizon_goal_deactivator.hpp"

struct MockSrtGoalDeactivator {
    MOCK_METHOD(void, deactivate, (const goal_lineage*));
};

struct MockGoalWeights {
    MOCK_METHOD(void, erase, (const goal_lineage*));
};

using TestHorizonGoalDeactivator = horizon_goal_deactivator<MockSrtGoalDeactivator, MockGoalWeights>;

struct HorizonGoalDeactivatorTest : public ::testing::Test {
    MockSrtGoalDeactivator mock_srt;
    MockGoalWeights goal_weights;
    goal_lineage gl{nullptr, 0};
    TestHorizonGoalDeactivator deactivator{mock_srt, goal_weights};
};

TEST_F(HorizonGoalDeactivatorTest, ErasesWeightThenDelegates) {
    testing::InSequence seq;
    EXPECT_CALL(goal_weights, erase(&gl)).Times(1);
    EXPECT_CALL(mock_srt, deactivate(&gl)).Times(1);
    deactivator.deactivate(&gl);
}
