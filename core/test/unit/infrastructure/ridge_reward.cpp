#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/ridge_reward.hpp"

using ::testing::Return;

struct MockGetDecisionCount {
    MOCK_METHOD(size_t, count, (), (const));
};

using test_ridge_reward_t = ridge_reward<MockGetDecisionCount>;

struct RidgeRewardTest : public ::testing::Test {
    MockGetDecisionCount decision_count;
    test_ridge_reward_t reward{decision_count};
};

TEST_F(RidgeRewardTest, ReturnsNegativeDecisionCount) {
    EXPECT_CALL(decision_count, count()).WillOnce(Return(4u));
    EXPECT_DOUBLE_EQ(reward.compute_mcts_reward(), -4.0);
}
