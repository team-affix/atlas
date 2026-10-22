#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/pud_query_head_nodes.hpp"

struct PudQueryHeadNodesTest : public ::testing::Test {
    pud_query_head_nodes head_nodes;
    pud_mhq_head_id head = 3;
    pud_mhq_head_id other_head = 4;
    pud_node_id node = 10;
};

TEST_F(PudQueryHeadNodesTest, GetReturnsTheStoredNode) {
    head_nodes.store(head, node);
    EXPECT_EQ(head_nodes.get(head), node);
}

TEST_F(PudQueryHeadNodesTest, ManyHeadsCanShareANode) {
    head_nodes.store(head, node);
    head_nodes.store(other_head, node);
    EXPECT_EQ(head_nodes.get(head), node);
    EXPECT_EQ(head_nodes.get(other_head), node);
}

TEST_F(PudQueryHeadNodesTest, GetOfAnUnstoredHeadThrows) {
    EXPECT_THROW(head_nodes.get(head), std::out_of_range);
}

TEST_F(PudQueryHeadNodesTest, SecondStoreOfSameHeadThrows) {
    head_nodes.store(head, node);
    EXPECT_THROW(head_nodes.store(head, node + 1), std::logic_error);
}
