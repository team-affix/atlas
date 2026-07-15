// ridge_set_up_sim: sets up MCTS before Basic solver-state setup.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/ridge_set_up_sim.hpp"

struct MockSetUpMcts {
    MOCK_METHOD(void, set_up, ());
};

struct MockSetUpSim {
    MOCK_METHOD(void, set_up, ());
};

using test_ridge_set_up_sim_t = ridge_set_up_sim<MockSetUpMcts, MockSetUpSim>;

struct RidgeSetUpSimTest : public ::testing::Test {
    MockSetUpMcts mock_set_up_mcts;
    MockSetUpSim mock_set_up;
    test_ridge_set_up_sim_t set_up{mock_set_up_mcts, mock_set_up};
};

TEST_F(RidgeSetUpSimTest, SetsUpMctsBeforeStateSetUp) {
    testing::InSequence seq;
    EXPECT_CALL(mock_set_up_mcts, set_up()).Times(1);
    EXPECT_CALL(mock_set_up, set_up()).Times(1);
    set_up.set_up();
}
