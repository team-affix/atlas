#include <gtest/gtest.h>
#include "infrastructure/sequencer.hpp"
#include "infrastructure/trail.hpp"

struct SequencerIntegrationTest : public ::testing::Test {
protected:
    trail t;
    sequencer<int> seq{t};
};

TEST_F(SequencerIntegrationTest, PopRevertsCounter) {
    EXPECT_EQ(seq.next(), 0);
    EXPECT_EQ(seq.next(), 1);
    t.push();
    EXPECT_EQ(seq.next(), 2);
    EXPECT_EQ(seq.next(), 3);
    EXPECT_EQ(seq.next(), 4);
    t.pop();
    EXPECT_EQ(seq.next(), 2);
}

TEST_F(SequencerIntegrationTest, TwoSequencersSharingTrailRevertIndependently) {
    sequencer<int> seq2{t};
    EXPECT_EQ(seq.next(), 0);
    EXPECT_EQ(seq.next(), 1);
    EXPECT_EQ(seq2.next(), 0);
    t.push();
    EXPECT_EQ(seq.next(), 2);
    EXPECT_EQ(seq2.next(), 1);
    EXPECT_EQ(seq2.next(), 2);
    t.pop();
    EXPECT_EQ(seq.next(), 2);
    EXPECT_EQ(seq2.next(), 1);
}
