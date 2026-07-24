// quell_set_up_sim: sets up MCTS before Quell solver-state setup.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/quell_set_up_sim.hpp"

struct MockSetUpMcts {
    MOCK_METHOD(void, set_up, ());
};

struct MockSetUpSim {
    MOCK_METHOD(void, set_up, ());
};

using test_quell_set_up_sim_t = quell_set_up_sim<MockSetUpMcts, MockSetUpSim>;

struct QuellSetUpSimTest : public ::testing::Test {
    MockSetUpMcts mock_set_up_mcts;
    MockSetUpSim mock_set_up;
    test_quell_set_up_sim_t set_up{mock_set_up_mcts, mock_set_up};
};

TEST_F(QuellSetUpSimTest, SetsUpMctsBeforeStateSetUp) {
    testing::InSequence seq;
    EXPECT_CALL(mock_set_up_mcts, set_up()).Times(1);
    EXPECT_CALL(mock_set_up, set_up()).Times(1);
    set_up.set_up();
}
