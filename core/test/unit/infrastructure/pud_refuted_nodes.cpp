#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/pud_refuted_nodes.hpp"

struct PudRefutedNodesTest : public ::testing::Test {
    pud_refuted_nodes refuted;
    pud_node node{};
    pud_node other{};
};

TEST_F(PudRefutedNodesTest, CheckRefutedIsFalseWhenNeverAdded) {
    EXPECT_FALSE(refuted.check_refuted(&node));
}

TEST_F(PudRefutedNodesTest, SecondSetRefutedThrows) {
    refuted.set_refuted(&node);
    EXPECT_THROW(refuted.set_refuted(&node), std::logic_error);
}

TEST_F(PudRefutedNodesTest, AddingOneNodeLeavesAnotherUnchecked) {
    refuted.set_refuted(&node);
    EXPECT_FALSE(refuted.check_refuted(&other));
}

TEST_F(PudRefutedNodesTest, SetRefutedThenCheckIsTrue) {
    refuted.set_refuted(&node);
    EXPECT_TRUE(refuted.check_refuted(&node));
}
