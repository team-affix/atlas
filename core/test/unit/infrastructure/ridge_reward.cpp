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

TEST_F(RidgeRewardTest, ZeroDecisionsGivesZeroReward) {
    // A proof found with no decisions is the best possible outcome, so it must sit
    // at the top of the reward range rather than wrapping around: count() is
    // unsigned, and negating before the cast to double would yield a huge
    // positive number for every non-zero count.
    EXPECT_CALL(decision_count, count()).WillOnce(Return(0u));
    EXPECT_DOUBLE_EQ(reward.compute_mcts_reward(), 0.0);
}

TEST_F(RidgeRewardTest, RewardIsStrictlyMoreNegativeAsDecisionsGrow) {
    // This monotonicity IS the ridge search objective: fewer decisions must always
    // score higher. Pinning one sample value cannot catch an inverted comparison.
    EXPECT_CALL(decision_count, count())
        .WillOnce(Return(0u))
        .WillOnce(Return(1u))
        .WillOnce(Return(2u))
        .WillOnce(Return(10u));

    const double r0 = reward.compute_mcts_reward();
    const double r1 = reward.compute_mcts_reward();
    const double r2 = reward.compute_mcts_reward();
    const double r10 = reward.compute_mcts_reward();

    EXPECT_GT(r0, r1);
    EXPECT_GT(r1, r2);
    EXPECT_GT(r2, r10);
}
