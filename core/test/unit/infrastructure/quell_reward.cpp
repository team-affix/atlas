// quell_reward: MCTS reward equals -remaining_work.get().

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/quell_reward.hpp"

using ::testing::AtLeast;
using ::testing::Return;

struct MockGetRemainingWork {
    MOCK_METHOD(double, get, (), (const));
};

using test_quell_reward_t = quell_reward<MockGetRemainingWork>;

struct QuellRewardTest : public ::testing::Test {
    MockGetRemainingWork remaining_work;
    test_quell_reward_t reward{remaining_work};

    static constexpr double kRemaining = 0.75;
};

TEST_F(QuellRewardTest, ReturnsNegatedRemainingWork) {
    EXPECT_CALL(remaining_work, get()).Times(AtLeast(1)).WillRepeatedly(Return(kRemaining));
    EXPECT_DOUBLE_EQ(reward.compute_mcts_reward(), -kRemaining);
}
