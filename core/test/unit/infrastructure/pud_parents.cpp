#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/pud_parents.hpp"

struct PudParentsTest : public ::testing::Test {
    pud_parents parents;
    pud_node node_a{};
    pud_node node_b{};
    pud_node parent_a{};
    pud_node parent_b{};
};

TEST_F(PudParentsTest, SecondStoreOfSameNodeThrows) {
    parents.store(&node_a, &parent_a);
    EXPECT_THROW(parents.store(&node_a, &parent_b), std::logic_error);
}

TEST_F(PudParentsTest, StoredNullptrParentReadsBackNullptr) {
    parents.store(&node_a, nullptr);
    EXPECT_EQ(parents.get(&node_a), nullptr);
}

TEST_F(PudParentsTest, TwoNodesKeepTheirOwnParents) {
    parents.store(&node_a, &parent_a);
    parents.store(&node_b, &parent_b);
    EXPECT_EQ(parents.get(&node_a), &parent_a);
    EXPECT_EQ(parents.get(&node_b), &parent_b);
}

TEST_F(PudParentsTest, StoreThenGetReturnsThatParent) {
    parents.store(&node_a, &parent_a);
    EXPECT_EQ(parents.get(&node_a), &parent_a);
}
