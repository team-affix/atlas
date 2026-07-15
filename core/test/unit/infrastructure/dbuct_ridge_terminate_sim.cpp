// dbuct_ridge_terminate_sim: snapshots ridge reward then delegates to dbuct_sim.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include "infrastructure/dbuct_ridge_terminate_sim.hpp"
#include "value_objects/lineage.hpp"

using ::testing::InSequence;
using ::testing::Return;

struct MockComputeMctsReward {
    MOCK_METHOD(double, compute_mcts_reward, (), (const));
};

struct MockSetValueDelta {
    MOCK_METHOD(void, set_value, (double));
};

struct MockTerminateDbuct {
    MOCK_METHOD((std::vector<const resolution_lineage*>), terminate, ());
};

using test_dbuct_ridge_terminate_sim_t = dbuct_ridge_terminate_sim<
    MockComputeMctsReward, MockSetValueDelta, MockTerminateDbuct>;

struct DbuctRidgeTerminateSimTest : public ::testing::Test {
    MockComputeMctsReward compute_mcts_reward;
    MockSetValueDelta set_value_delta;
    MockTerminateDbuct terminate_dbuct;
    test_dbuct_ridge_terminate_sim_t terminate{
        compute_mcts_reward, set_value_delta, terminate_dbuct};
};

TEST_F(DbuctRidgeTerminateSimTest, SetsRewardThenDelegatesAndForwardsEliminations) {
    resolution_lineage rl{nullptr, 0};
    std::vector<const resolution_lineage*> elims{&rl};

    InSequence seq;
    EXPECT_CALL(compute_mcts_reward, compute_mcts_reward()).WillOnce(Return(-4.0));
    EXPECT_CALL(set_value_delta, set_value(-4.0)).Times(1);
    EXPECT_CALL(terminate_dbuct, terminate()).WillOnce(Return(elims));

    EXPECT_EQ(terminate.terminate(), elims);
}
