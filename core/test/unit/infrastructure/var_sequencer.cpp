// Variable sequencer: monotonic uint32_t ids with trail-backed undo via sequencer.
// next() must return 0, 1, 2 in order and log each step on the trail.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/var_sequencer.hpp"
#include "../../../core/hpp/utility/i_trail.hpp"

using ::testing::_;

struct MockTrail : public i_trail {
    MOCK_METHOD(void, push, (), (override));
    MOCK_METHOD(void, pop, (), (override));
    MOCK_METHOD(void, log, ((std::unique_ptr<i_backtrackable>)), (override));
};

struct VarSequencerTest : public ::testing::Test {
    MockTrail trail;
    var_sequencer seq{trail};
};

TEST_F(VarSequencerTest, NextReturnsIncreasingIds) {
    EXPECT_CALL(trail, log(_)).Times(3);
    EXPECT_EQ(seq.next(), 0u);
    EXPECT_EQ(seq.next(), 1u);
    EXPECT_EQ(seq.next(), 2u);
}
