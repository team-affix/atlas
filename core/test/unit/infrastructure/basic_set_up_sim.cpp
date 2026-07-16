// basic_set_up_sim: delegates set_up to the Basic solver's state setup.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/basic_set_up_sim.hpp"

struct MockSetUpSim {
    MOCK_METHOD(void, set_up, ());
};

using test_basic_set_up_sim_t = basic_set_up_sim<MockSetUpSim>;

struct BasicSetUpSimTest : public ::testing::Test {
    MockSetUpSim mock_set_up;
    test_basic_set_up_sim_t set_up{mock_set_up};
};

TEST_F(BasicSetUpSimTest, DelegatesToInnerSetUp) {
    EXPECT_CALL(mock_set_up, set_up()).Times(1);
    set_up.set_up();
}
