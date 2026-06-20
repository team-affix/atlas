// sequencer allocates monotonic indices with trail-backed undo. Unit tests mock
// the log interface and assert next() returns 0,1,2 with one log call per allocation.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/sequencer.hpp"
#include "interfaces/i_backtrackable.hpp"

using ::testing::_;
using ::testing::NiceMock;

struct MockTrail {
    MOCK_METHOD(void, log, ((std::unique_ptr<i_backtrackable>)));
};

struct SequencerTest : public ::testing::Test {
protected:
    NiceMock<MockTrail> trail;
    sequencer<int, MockTrail> seq{trail, 0};
};

TEST_F(SequencerTest, NextReturns0Then1Then2InOrder) {
    EXPECT_CALL(trail, log(_)).Times(3);

    EXPECT_EQ(seq.next(), 0);
    EXPECT_EQ(seq.next(), 1);
    EXPECT_EQ(seq.next(), 2);
}

TEST_F(SequencerTest, NextStartsFromExplicitInitial) {
    sequencer<int, MockTrail> seq_from_five{trail, 5};
    EXPECT_CALL(trail, log(_)).Times(3);

    EXPECT_EQ(seq_from_five.next(), 5);
    EXPECT_EQ(seq_from_five.next(), 6);
    EXPECT_EQ(seq_from_five.next(), 7);
}
