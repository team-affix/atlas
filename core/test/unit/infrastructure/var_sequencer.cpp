// Variable sequencer: monotonic uint32_t ids with trail-backed undo via sequencer.
// next() must return 0, 1, 2 in order and log each step on the trail.

#include <gtest/gtest.h>
#include "locator_fixture.hpp"
#include <gmock/gmock.h>
#include "infrastructure/var_sequencer.hpp"
#include "interfaces/i_log_to_current_trail_frame.hpp"

using ::testing::_;

struct MockTrail : public i_log_to_current_trail_frame {
    MOCK_METHOD(void, log, ((std::unique_ptr<i_backtrackable>)), (override));
};

struct VarSequencerTest : public ::testing::Test {
    locator loc;
    MockTrail trail;
    var_sequencer seq;

    VarSequencerTest()
        : seq(bind_and_make<var_sequencer, i_log_to_current_trail_frame>(loc, trail)) {}
};

TEST_F(VarSequencerTest, NextReturnsIncreasingIds) {
    EXPECT_CALL(trail, log(_)).Times(3);
    EXPECT_EQ(seq.next(), 0u);
    EXPECT_EQ(seq.next(), 1u);
    EXPECT_EQ(seq.next(), 2u);
}
