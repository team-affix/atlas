#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/pud_children.hpp"

struct PudChildrenTest : public ::testing::Test {
    pud_children children;
    pud_node node{};
    pud_node other{};
    pud_node child_a{};
    pud_node child_b{};
    pud_node child_c{};
};

TEST_F(PudChildrenTest, SecondStoreOfSameNodeThrows) {
    children.store(&node, {&child_a});
    EXPECT_THROW(children.store(&node, {&child_b}), std::logic_error);
}

TEST_F(PudChildrenTest, StoredEmptySequenceReadsBackEmpty) {
    children.store(&node, {});
    EXPECT_TRUE(children.get(&node).empty());
}

TEST_F(PudChildrenTest, SequenceStaysValidAfterOtherNodesAreStored) {
    children.store(&node, {&child_a, &child_b});
    const std::vector<const pud_node*>& held = children.get(&node);
    children.store(&other, {&child_c});
    ASSERT_EQ(held.size(), 2u);
    EXPECT_EQ(held[0], &child_a);
    EXPECT_EQ(held[1], &child_b);
}

TEST_F(PudChildrenTest, GetReturnsStoredSequenceInOrder) {
    children.store(&node, {&child_a, &child_b, &child_c});
    const std::vector<const pud_node*>& got = children.get(&node);
    ASSERT_EQ(got.size(), 3u);
    EXPECT_EQ(got[0], &child_a);
    EXPECT_EQ(got[1], &child_b);
    EXPECT_EQ(got[2], &child_c);
}
