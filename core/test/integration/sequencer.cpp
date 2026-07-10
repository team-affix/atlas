#include <gtest/gtest.h>
#include "infrastructure/sequencer.hpp"

struct SequencerIntegrationTest : public ::testing::Test {
protected:
    sequencer<int> seq{0};
};

TEST_F(SequencerIntegrationTest, PopRevertsCounter) {
    EXPECT_EQ(seq.next(), 0);
    EXPECT_EQ(seq.next(), 1);
    seq.push_frame();
    EXPECT_EQ(seq.next(), 2);
    EXPECT_EQ(seq.next(), 3);
    EXPECT_EQ(seq.next(), 4);
    seq.pop_frame();
    EXPECT_EQ(seq.next(), 2);
}

TEST_F(SequencerIntegrationTest, TwoSequencersRevertIndependently) {
    sequencer<int> seq2{0};
    EXPECT_EQ(seq.next(), 0);
    EXPECT_EQ(seq.next(), 1);
    EXPECT_EQ(seq2.next(), 0);
    seq.push_frame();
    EXPECT_EQ(seq.next(), 2);
    EXPECT_EQ(seq2.next(), 1);
    EXPECT_EQ(seq2.next(), 2);
    seq.pop_frame();
    EXPECT_EQ(seq.next(), 2);
    EXPECT_EQ(seq2.next(), 3);
}
