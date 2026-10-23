#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/pud_node_live_child_counts.hpp"
#include "pud_test_node.hpp"

namespace {

struct PudNodeLiveChildCountsTest : public ::testing::Test {
    pud_node_live_child_counts counts;
};

}

TEST_F(PudNodeLiveChildCountsTest, EachIncrementAddsOneLiveChildAndEachDecrementReturnsTheCountLeft) {
    counts.increment(pud_test_node(4));
    counts.increment(pud_test_node(4));
    counts.increment(pud_test_node(4));
    EXPECT_EQ(counts.decrement(pud_test_node(4)), 2u);
    EXPECT_EQ(counts.decrement(pud_test_node(4)), 1u);
    EXPECT_EQ(counts.decrement(pud_test_node(4)), 0u);
}

TEST_F(PudNodeLiveChildCountsTest, CountsOfDifferentNodesAreIndependent) {
    counts.increment(pud_test_node(1));
    counts.increment(pud_test_node(1));
    counts.increment(pud_test_node(5));
    counts.increment(pud_test_node(5));
    counts.decrement(pud_test_node(5));
    EXPECT_EQ(counts.decrement(pud_test_node(1)), 1u);
    EXPECT_EQ(counts.decrement(pud_test_node(5)), 0u);
}

TEST_F(PudNodeLiveChildCountsTest, TheSameLocalIdInTwoModulesCountsSeparately) {
    counts.increment(pud_global_node_id{.module = 1, .node = 0});
    counts.increment(pud_global_node_id{.module = 2, .node = 0});
    counts.increment(pud_global_node_id{.module = 2, .node = 0});
    EXPECT_EQ(counts.decrement(pud_global_node_id{.module = 1, .node = 0}), 0u);
    EXPECT_EQ(counts.decrement(pud_global_node_id{.module = 2, .node = 0}), 1u);
}

TEST_F(PudNodeLiveChildCountsTest, ANodeWithNoActivatedChildCannotLoseOne) {
    EXPECT_THROW(counts.decrement(pud_test_node(3)), std::out_of_range);
}

TEST_F(PudNodeLiveChildCountsTest, DecrementingBelowZeroFails) {
    counts.increment(pud_test_node(3));
    counts.decrement(pud_test_node(3));
    EXPECT_THROW(counts.decrement(pud_test_node(3)), std::logic_error);
}
