// set_up_sim: opens the solver frame the whole simulation runs inside.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/set_up_sim.hpp"

namespace {

struct MockPushFrame {
    MOCK_METHOD(void, push_frame, ());
};

} // namespace

using test_set_up_sim_t = set_up_sim<MockPushFrame>;

struct SetUpSimTest : public ::testing::Test {
    MockPushFrame push_frame;
    test_set_up_sim_t sut{push_frame};
};

TEST_F(SetUpSimTest, PushesSolverFrame) {
    // Everything the simulation mutates is journaled into this frame, so a
    // missing push leaves tear_down popping the base frame instead.
    EXPECT_CALL(push_frame, push_frame()).Times(1);
    sut.set_up();
}
