// horizon_reward: MCTS reward equals cumulative grounded weight.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/horizon_reward.hpp"

using ::testing::Return;

struct MockCumulativeGroundedWeight {
    MOCK_METHOD(double, get, (), (const));
};

using test_horizon_reward_t = horizon_reward<MockCumulativeGroundedWeight>;

struct HorizonRewardTest : public ::testing::Test {
    MockCumulativeGroundedWeight grounded_weight;
    test_horizon_reward_t reward{grounded_weight};

    static constexpr double kCgw = 0.75;
};

TEST_F(HorizonRewardTest, ReturnsGroundedWeight) {
    EXPECT_CALL(grounded_weight, get()).WillOnce(Return(kCgw));
    EXPECT_DOUBLE_EQ(reward.compute_mcts_reward(), kCgw);
}
