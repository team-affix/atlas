// non_backtracking_var_sequencer: monotonic var indices without trail undo.
// peek() reports how many indices have been issued (next value next() would return).

#include <gtest/gtest.h>
#include "infrastructure/non_backtracking_var_sequencer.hpp"

TEST(NonBacktrackingVarSequencerTest, PeekZeroInitially) {
    non_backtracking_var_sequencer seq;
    EXPECT_EQ(seq.peek(), 0u);
}

TEST(NonBacktrackingVarSequencerTest, NextAndPeekTrackIssuedCount) {
    non_backtracking_var_sequencer seq;
    EXPECT_EQ(seq.next(), 0u);
    EXPECT_EQ(seq.peek(), 1u);
    EXPECT_EQ(seq.next(), 1u);
    EXPECT_EQ(seq.peek(), 2u);
    EXPECT_EQ(seq.next(), 2u);
    EXPECT_EQ(seq.peek(), 3u);
    EXPECT_EQ(seq.next(), 3u);
    EXPECT_EQ(seq.peek(), 4u);
}

TEST(NonBacktrackingVarSequencerTest, StartsFromExplicitInitial) {
    non_backtracking_var_sequencer seq{5};
    EXPECT_EQ(seq.peek(), 5u);
    EXPECT_EQ(seq.next(), 5u);
    EXPECT_EQ(seq.peek(), 6u);
}
