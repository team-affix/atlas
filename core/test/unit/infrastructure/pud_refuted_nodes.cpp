#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/pud_refuted_nodes.hpp"

struct PudRefutedNodesTest : public ::testing::Test {
    pud_refuted_nodes refuted;
    pud_node_id node = 1;
    pud_node_id other = 2;
};

TEST_F(PudRefutedNodesTest, CheckRefutedIsFalseWhenNeverAdded) {
    EXPECT_FALSE(refuted.check_refuted(node));
}

TEST_F(PudRefutedNodesTest, SecondMarkRefutedThrows) {
    refuted.mark_refuted(node);
    EXPECT_THROW(refuted.mark_refuted(node), std::logic_error);
}

TEST_F(PudRefutedNodesTest, AddingOneNodeLeavesAnotherUnchecked) {
    refuted.mark_refuted(node);
    EXPECT_FALSE(refuted.check_refuted(other));
}

TEST_F(PudRefutedNodesTest, MarkRefutedThenCheckIsTrue) {
    refuted.mark_refuted(node);
    EXPECT_TRUE(refuted.check_refuted(node));
}
