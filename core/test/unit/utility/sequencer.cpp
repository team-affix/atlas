#include <gtest/gtest.h>
#include "../../../core/hpp/utility/sequencer.hpp"
#include "../../../core/hpp/utility/trail.hpp"

class SequencerTest : public ::testing::Test {
protected:
    trail t;
    sequencer<int> seq{t};
};

TEST_F(SequencerTest, NextReturns0Then1Then2InOrder) {
    EXPECT_EQ(seq.next(), 0);
    EXPECT_EQ(seq.next(), 1);
    EXPECT_EQ(seq.next(), 2);
}

TEST_F(SequencerTest, PopRevertsCounter) {
    seq.next(); seq.next();
    t.push();
    seq.next(); seq.next(); seq.next();
    t.pop();
    EXPECT_EQ(seq.next(), 2);
}

TEST_F(SequencerTest, TwoSequencersSharingTrailRevertIndependently) {
    sequencer<int> seq2(t);
    seq.next(); seq.next();
    seq2.next();
    t.push();
    seq.next();
    seq2.next(); seq2.next();
    t.pop();
    EXPECT_EQ(seq.next(), 2);
    EXPECT_EQ(seq2.next(), 1);
}
