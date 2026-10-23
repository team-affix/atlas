#include <gtest/gtest.h>
#include "infrastructure/pud_node_live_child_counts.hpp"
#include "pud_test_node.hpp"

namespace {

struct PudNodeLiveChildCountsTest : public ::testing::Test {
    pud_node_live_child_counts counts;
};

}

TEST_F(PudNodeLiveChildCountsTest, AStoredCountReadsBack) {
    counts.store(pud_test_node(4), 3);
    EXPECT_EQ(counts.get(pud_test_node(4)), 3u);
}

TEST_F(PudNodeLiveChildCountsTest, DecrementReturnsTheCountLeft) {
    counts.store(pud_test_node(0), 2);
    EXPECT_EQ(counts.decrement(pud_test_node(0)), 1u);
    EXPECT_EQ(counts.decrement(pud_test_node(0)), 0u);
    EXPECT_EQ(counts.get(pud_test_node(0)), 0u);
}

TEST_F(PudNodeLiveChildCountsTest, CountsOfDifferentNodesAreIndependent) {
    counts.store(pud_test_node(1), 2);
    counts.store(pud_test_node(5), 2);
    counts.decrement(pud_test_node(5));
    EXPECT_EQ(counts.get(pud_test_node(1)), 2u);
    EXPECT_EQ(counts.get(pud_test_node(5)), 1u);
}
