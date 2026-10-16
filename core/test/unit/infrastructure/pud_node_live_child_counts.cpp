#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/pud_node_live_child_counts.hpp"

struct PudNodeLiveChildCountsTest : public ::testing::Test {
    pud_node_live_child_counts counts;
    pud_node_id node = 1;
    pud_node_id other = 2;
};

TEST_F(PudNodeLiveChildCountsTest, GetReturnsTheStoredCount) {
    counts.store(node, 3);
    EXPECT_EQ(counts.get(node), 3u);
}

TEST_F(PudNodeLiveChildCountsTest, GetOfAnUnstoredNodeThrows) {
    EXPECT_THROW(counts.get(node), std::out_of_range);
}

TEST_F(PudNodeLiveChildCountsTest, DecrementReturnsTheRemainingCount) {
    counts.store(node, 3);
    EXPECT_EQ(counts.decrement(node), 2u);
    EXPECT_EQ(counts.get(node), 2u);
}

TEST_F(PudNodeLiveChildCountsTest, DecrementingTheLastLiveChildReachesZero) {
    counts.store(node, 1);
    EXPECT_EQ(counts.decrement(node), 0u);
    EXPECT_EQ(counts.get(node), 0u);
}

TEST_F(PudNodeLiveChildCountsTest, DecrementingAtZeroThrows) {
    counts.store(node, 0);
    EXPECT_THROW(counts.decrement(node), std::logic_error);
}

TEST_F(PudNodeLiveChildCountsTest, DecrementingAnUnstoredNodeThrows) {
    EXPECT_THROW(counts.decrement(node), std::out_of_range);
}

TEST_F(PudNodeLiveChildCountsTest, SecondStoreOfSameNodeThrows) {
    counts.store(node, 1);
    EXPECT_THROW(counts.store(node, 4), std::logic_error);
}

TEST_F(PudNodeLiveChildCountsTest, NodesKeepTheirOwnCounts) {
    counts.store(node, 2);
    counts.store(other, 5);
    counts.decrement(node);
    EXPECT_EQ(counts.get(node), 1u);
    EXPECT_EQ(counts.get(other), 5u);
}
