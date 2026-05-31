// cdcl_sequencer allocates monotonic size_t indices via sequencer, resolving
// i_log_to_current_trail_frame from the locator. Unit tests mock the trail and
// assert next() returns 0, 1, 2 with one log call per allocation.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "locator_fixture.hpp"
#include "infrastructure/cdcl_sequencer.hpp"
#include "interfaces/i_log_to_current_trail_frame.hpp"

using ::testing::_;

struct MockTrail : public i_log_to_current_trail_frame {
    MOCK_METHOD(void, log, ((std::unique_ptr<i_backtrackable>)), (override));
};

struct CdclSequencerTest : public ::testing::Test {
    locator loc;
    MockTrail trail;
    cdcl_sequencer seq;

    CdclSequencerTest()
        : seq(bind_and_make<cdcl_sequencer, i_log_to_current_trail_frame>(loc, trail)) {}
};

TEST_F(CdclSequencerTest, NextReturnsIncreasingIds) {
    EXPECT_CALL(trail, log(_)).Times(3);
    EXPECT_EQ(seq.next(), 0u);
    EXPECT_EQ(seq.next(), 1u);
    EXPECT_EQ(seq.next(), 2u);
}
