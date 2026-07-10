// sequencer allocates monotonic indices with frame-backed undo.

#include <gtest/gtest.h>
#include "infrastructure/sequencer.hpp"

struct SequencerTest : public ::testing::Test {
protected:
    sequencer<int> seq{0};
};

TEST_F(SequencerTest, NextReturns0Then1Then2InOrder) {
    EXPECT_EQ(seq.next(), 0);
    EXPECT_EQ(seq.next(), 1);
    EXPECT_EQ(seq.next(), 2);
}

TEST_F(SequencerTest, NextStartsFromExplicitInitial) {
    sequencer<int> seq_from_five{5};
    EXPECT_EQ(seq_from_five.next(), 5);
    EXPECT_EQ(seq_from_five.next(), 6);
    EXPECT_EQ(seq_from_five.next(), 7);
}

TEST_F(SequencerTest, PopRevertsCounter) {
    EXPECT_EQ(seq.next(), 0);
    EXPECT_EQ(seq.next(), 1);
    seq.push_frame();
    EXPECT_EQ(seq.next(), 2);
    seq.pop_frame();
    EXPECT_EQ(seq.next(), 2);
}
