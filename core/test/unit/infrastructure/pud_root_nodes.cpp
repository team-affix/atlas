#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/pud_root_nodes.hpp"

struct PudRootNodesTest : public ::testing::Test {
    pud_root_nodes roots;
    pud_node_id node = 1;
    pud_node_id other = 2;
};

TEST_F(PudRootNodesTest, CheckRootIsFalseWhenNeverMarked) {
    EXPECT_FALSE(roots.check_root(node));
}

TEST_F(PudRootNodesTest, MarkRootThenCheckIsTrue) {
    roots.mark_root(node);
    EXPECT_TRUE(roots.check_root(node));
}

TEST_F(PudRootNodesTest, MarkingOneRootDoesNotMarkAnother) {
    roots.mark_root(node);
    EXPECT_FALSE(roots.check_root(other));
}

TEST_F(PudRootNodesTest, SecondMarkRootThrows) {
    roots.mark_root(node);
    EXPECT_THROW(roots.mark_root(node), std::logic_error);
}

TEST_F(PudRootNodesTest, GetIsEmptyBeforeAnyRootIsMarked) {
    EXPECT_TRUE(roots.get().empty());
}

TEST_F(PudRootNodesTest, GetListsRootsInMarkingOrder) {
    roots.mark_root(other);
    roots.mark_root(node);
    EXPECT_EQ(roots.get(), (std::vector<pud_node_id>{other, node}));
}
