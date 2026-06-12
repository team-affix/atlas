// horizon_reward: MCTS reward equals cumulative grounded weight.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "locator_fixture.hpp"
#include "infrastructure/horizon_reward.hpp"
#include "interfaces/i_get_grounded_weight.hpp"

using ::testing::Return;

struct MockGetGroundedWeight : public i_get_grounded_weight {
    MOCK_METHOD(double, get, (), (const, override));
};

struct HorizonRewardTest : public ::testing::Test {
    locator loc;
    MockGetGroundedWeight grounded_weight;
    horizon_reward reward;

    static constexpr double kCgw = 0.75;

    HorizonRewardTest() : reward(init_reward()) {}

    horizon_reward init_reward() {
        loc.bind_as<i_get_grounded_weight>(grounded_weight);
        return horizon_reward{loc};
    }
};

TEST_F(HorizonRewardTest, ReturnsGroundedWeight) {
    EXPECT_CALL(grounded_weight, get()).WillOnce(Return(kCgw));
    EXPECT_DOUBLE_EQ(reward.compute_mcts_reward(), kCgw);
}
