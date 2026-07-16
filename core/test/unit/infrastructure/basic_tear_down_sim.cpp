// basic_tear_down_sim: delegates tear_down to the Basic solver's state teardown.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/basic_tear_down_sim.hpp"

struct MockTearDownSim {
    MOCK_METHOD(void, tear_down, ());
};

using test_basic_tear_down_sim_t = basic_tear_down_sim<MockTearDownSim>;

struct BasicTearDownSimTest : public ::testing::Test {
    MockTearDownSim mock_tear_down;
    test_basic_tear_down_sim_t tear_down{mock_tear_down};
};

TEST_F(BasicTearDownSimTest, DelegatesToInnerTearDown) {
    EXPECT_CALL(mock_tear_down, tear_down()).Times(1);
    tear_down.tear_down();
}
