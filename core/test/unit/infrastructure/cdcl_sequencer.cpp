// CDCL sequencer: monotonic size_t decision indices with trail logging. next() must
// yield 0, 1, 2 in order for successive calls.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/cdcl_sequencer.hpp"
#include "../../../core/hpp/utility/i_trail.hpp"

using ::testing::_;

struct MockTrail : public i_trail {
    MOCK_METHOD(void, push, (), (override));
    MOCK_METHOD(void, pop, (), (override));
    MOCK_METHOD(void, log, ((std::unique_ptr<i_backtrackable>)), (override));
};

struct CdclSequencerTest : public ::testing::Test {
    MockTrail trail;
    cdcl_sequencer seq{trail};
};

TEST_F(CdclSequencerTest, NextReturnsIncreasingIndices) {
    EXPECT_CALL(trail, log(_)).Times(3);
    EXPECT_EQ(seq.next(), 0u);
    EXPECT_EQ(seq.next(), 1u);
    EXPECT_EQ(seq.next(), 2u);
}
