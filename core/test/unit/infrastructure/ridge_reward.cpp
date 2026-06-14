#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "locator_fixture.hpp"
#include "infrastructure/ridge_reward.hpp"
#include "interfaces/i_get_decision_count.hpp"

using ::testing::Return;

struct MockGetDecisionCount : public i_get_decision_count {
    MOCK_METHOD(size_t, count, (), (const, override));
};

struct RidgeRewardTest : public ::testing::Test {
    locator loc;
    MockGetDecisionCount decision_count;
    ridge_reward reward;

    RidgeRewardTest() : reward(init_reward()) {}

    ridge_reward init_reward() {
        loc.bind_as<i_get_decision_count>(decision_count);
        return ridge_reward{loc};
    }
};

TEST_F(RidgeRewardTest, ReturnsNegativeDecisionCount) {
    EXPECT_CALL(decision_count, count()).WillOnce(Return(4u));
    EXPECT_DOUBLE_EQ(reward.compute_mcts_reward(), -4.0);
}
