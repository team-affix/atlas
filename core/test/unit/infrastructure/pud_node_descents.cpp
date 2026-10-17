#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/pud_node_descents.hpp"

struct PudNodeDescentsTest : public ::testing::Test {
    pud_node_descents<int> descents;
    pud_node_id node = 1;
    pud_node_id other = 2;
};

TEST_F(PudNodeDescentsTest, GetReturnsTheStoredDescent) {
    descents.store(node, 7);
    EXPECT_EQ(descents.get(node), 7);
}

TEST_F(PudNodeDescentsTest, NodesKeepTheirOwnDescents) {
    descents.store(node, 7);
    descents.store(other, 9);
    EXPECT_EQ(descents.get(node), 7);
    EXPECT_EQ(descents.get(other), 9);
}

TEST_F(PudNodeDescentsTest, GetOfAnUnstoredNodeThrows) {
    EXPECT_THROW(descents.get(node), std::out_of_range);
}

TEST_F(PudNodeDescentsTest, ErasedDescentIsGone) {
    descents.store(node, 7);
    descents.erase(node);
    EXPECT_THROW(descents.get(node), std::out_of_range);
}

TEST_F(PudNodeDescentsTest, ErasingAnUnstoredNodeIsANoop) {
    descents.store(other, 9);
    descents.erase(node);
    EXPECT_EQ(descents.get(other), 9);
}

TEST_F(PudNodeDescentsTest, NodeCanBeStoredAgainAfterErasing) {
    descents.store(node, 7);
    descents.erase(node);
    descents.store(node, 8);
    EXPECT_EQ(descents.get(node), 8);
}

TEST_F(PudNodeDescentsTest, SecondStoreOfSameNodeThrows) {
    descents.store(node, 7);
    EXPECT_THROW(descents.store(node, 8), std::logic_error);
}

TEST_F(PudNodeDescentsTest, CheckDescentFollowsStoreAndErase) {
    EXPECT_FALSE(descents.check_descent(node));
    descents.store(node, 7);
    EXPECT_TRUE(descents.check_descent(node));
    EXPECT_FALSE(descents.check_descent(other));
    descents.erase(node);
    EXPECT_FALSE(descents.check_descent(node));
}
