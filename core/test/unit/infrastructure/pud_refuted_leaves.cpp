#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/pud_refuted_leaves.hpp"

struct PudRefutedLeavesTest : public ::testing::Test {
    pud_refuted_leaves refuted;
    pud_node_id leaf = 1;
    pud_node_id other = 2;
};

TEST_F(PudRefutedLeavesTest, CheckRefutedIsFalseWhenNeverMarked) {
    EXPECT_FALSE(refuted.check_refuted(leaf));
}

TEST_F(PudRefutedLeavesTest, MarkRefutedThenCheckIsTrue) {
    refuted.mark_refuted(leaf);
    EXPECT_TRUE(refuted.check_refuted(leaf));
}

TEST_F(PudRefutedLeavesTest, MarkingOneLeafDoesNotMarkAnother) {
    refuted.mark_refuted(leaf);
    EXPECT_FALSE(refuted.check_refuted(other));
}

TEST_F(PudRefutedLeavesTest, SecondMarkRefutedThrows) {
    refuted.mark_refuted(leaf);
    EXPECT_THROW(refuted.mark_refuted(leaf), std::logic_error);
}
