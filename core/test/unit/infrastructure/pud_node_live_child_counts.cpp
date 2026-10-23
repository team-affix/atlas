#include <gtest/gtest.h>
#include "infrastructure/pud_node_live_child_counts.hpp"

namespace {

struct PudNodeLiveChildCountsTest : public ::testing::Test {
    pud_node_live_child_counts counts;
};

}

TEST_F(PudNodeLiveChildCountsTest, AStoredCountReadsBack) {
    counts.store(4, 3);
    EXPECT_EQ(counts.get(4), 3u);
}

TEST_F(PudNodeLiveChildCountsTest, DecrementReturnsTheCountLeft) {
    counts.store(0, 2);
    EXPECT_EQ(counts.decrement(0), 1u);
    EXPECT_EQ(counts.decrement(0), 0u);
    EXPECT_EQ(counts.get(0), 0u);
}

TEST_F(PudNodeLiveChildCountsTest, CountsOfDifferentNodesAreIndependent) {
    counts.store(1, 2);
    counts.store(5, 2);
    counts.decrement(5);
    EXPECT_EQ(counts.get(1), 2u);
    EXPECT_EQ(counts.get(5), 1u);
}
