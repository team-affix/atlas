#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/pud_node_parents.hpp"

struct PudNodeParentsTest : public ::testing::Test {
    pud_node_parents parents;
    pud_node_id node = 1;
    pud_node_id other = 2;
    pud_node_id parent = 10;
    pud_node_id other_parent = 11;
};

TEST_F(PudNodeParentsTest, GetOfANodeWithNoStoredParentThrows) {
    EXPECT_THROW(parents.get(node), std::out_of_range);
}

TEST_F(PudNodeParentsTest, GetReturnsTheStoredParent) {
    parents.store(node, parent);
    EXPECT_EQ(parents.get(node), parent);
}

TEST_F(PudNodeParentsTest, NodesKeepTheirOwnParents) {
    parents.store(node, parent);
    parents.store(other, other_parent);
    EXPECT_EQ(parents.get(node), parent);
    EXPECT_EQ(parents.get(other), other_parent);
}

TEST_F(PudNodeParentsTest, SiblingsShareAParent) {
    parents.store(node, parent);
    parents.store(other, parent);
    EXPECT_EQ(parents.get(node), parent);
    EXPECT_EQ(parents.get(other), parent);
}

TEST_F(PudNodeParentsTest, StoringANodeDoesNotGiveItsParentAParent) {
    parents.store(node, parent);
    EXPECT_THROW(parents.get(parent), std::out_of_range);
}

TEST_F(PudNodeParentsTest, SecondStoreOfSameNodeThrows) {
    parents.store(node, parent);
    EXPECT_THROW(parents.store(node, other_parent), std::logic_error);
}
