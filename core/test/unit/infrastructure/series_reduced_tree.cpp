// series_reduced_tree maintains a series-reduced forest (no node with exactly one child).
// insert registers isolated nodes; link wires children to a leaf parent; unlink/erase remove edges or singletons.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdexcept>
#include <unordered_set>
#include "infrastructure/series_reduced_tree.hpp"

using ::testing::UnorderedElementsAre;

struct SeriesReducedTreeTest : public ::testing::Test {
    series_reduced_tree<int> tree;
};

TEST_F(SeriesReducedTreeTest, InsertAddsIsolatedNode) {
    ASSERT_TRUE(tree.insert(1));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(1));
    ASSERT_TRUE(tree.erase(1));
    EXPECT_TRUE(tree.roots().empty());
}

TEST_F(SeriesReducedTreeTest, InsertDuplicateReturnsFalse) {
    ASSERT_TRUE(tree.insert(1));
    EXPECT_FALSE(tree.insert(1));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(1));
}

TEST_F(SeriesReducedTreeTest, LinkMultipleChildren) {
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.insert(20));
    ASSERT_TRUE(tree.link(1, {10, 20}));
    EXPECT_THAT(tree.children(1), UnorderedElementsAre(10, 20));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(1));
}

TEST_F(SeriesReducedTreeTest, LinkSingletonCollapsesParent) {
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.link(1, {10}));
    EXPECT_THROW((void)tree.children(1), std::out_of_range);
    EXPECT_THROW((void)tree.children(10), std::out_of_range);
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(10));
    EXPECT_FALSE(tree.erase(1));
}

TEST_F(SeriesReducedTreeTest, LinkChainCollapses) {
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.insert(20));
    ASSERT_TRUE(tree.insert(100));
    ASSERT_TRUE(tree.insert(1000));
    ASSERT_TRUE(tree.link(1, {10, 20}));
    ASSERT_TRUE(tree.link(10, {100}));
    ASSERT_TRUE(tree.link(100, {1000}));
    EXPECT_THAT(tree.children(1), UnorderedElementsAre(1000, 20));
    EXPECT_THROW((void)tree.children(10), std::out_of_range);
    EXPECT_THROW((void)tree.children(100), std::out_of_range);
}

TEST_F(SeriesReducedTreeTest, MultipleRoots) {
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.insert(20));
    ASSERT_TRUE(tree.insert(2));
    ASSERT_TRUE(tree.insert(30));
    ASSERT_TRUE(tree.insert(40));
    ASSERT_TRUE(tree.link(1, {10, 20}));
    ASSERT_TRUE(tree.link(2, {30, 40}));
    EXPECT_THAT(tree.children(1), UnorderedElementsAre(10, 20));
    EXPECT_THAT(tree.children(2), UnorderedElementsAre(30, 40));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(1, 2));
}

TEST_F(SeriesReducedTreeTest, LinkFailsParentNotLeaf) {
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.insert(20));
    ASSERT_TRUE(tree.link(1, {10, 20}));
    ASSERT_TRUE(tree.insert(30));
    EXPECT_FALSE(tree.link(1, {30}));
    EXPECT_THAT(tree.children(1), UnorderedElementsAre(10, 20));
}

TEST_F(SeriesReducedTreeTest, LinkFailsChildNotRoot) {
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.insert(20));
    ASSERT_TRUE(tree.link(1, {10, 20}));
    ASSERT_TRUE(tree.insert(2));
    EXPECT_FALSE(tree.link(2, {10}));
    EXPECT_THAT(tree.children(1), UnorderedElementsAre(10, 20));
}

TEST_F(SeriesReducedTreeTest, LinkFailsEmptyChildren) {
    ASSERT_TRUE(tree.insert(1));
    EXPECT_FALSE(tree.link(1, {}));
}

TEST_F(SeriesReducedTreeTest, UnlinkRestoresChildAsRoot) {
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.insert(20));
    ASSERT_TRUE(tree.link(1, {10, 20}));
    ASSERT_TRUE(tree.unlink(10));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(1, 10));
    EXPECT_THAT(tree.children(1), UnorderedElementsAre(20));
    ASSERT_TRUE(tree.erase(10));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(1));
}

TEST_F(SeriesReducedTreeTest, UnlinkPromoteViaEraseParent) {
    ASSERT_TRUE(tree.insert(0));
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(2));
    ASSERT_TRUE(tree.link(0, {1, 2}));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.insert(20));
    ASSERT_TRUE(tree.link(1, {10, 20}));
    ASSERT_TRUE(tree.unlink(2));
    ASSERT_TRUE(tree.erase(0));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(1, 2));
    EXPECT_THAT(tree.children(1), UnorderedElementsAre(10, 20));
}

TEST_F(SeriesReducedTreeTest, UnlinkFailsNoEdge) {
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.insert(20));
    ASSERT_TRUE(tree.link(1, {10, 20}));
    EXPECT_FALSE(tree.unlink(99));
    ASSERT_TRUE(tree.insert(30));
    EXPECT_FALSE(tree.unlink(30));
}

TEST_F(SeriesReducedTreeTest, EraseIsolatedSucceeds) {
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.erase(1));
    EXPECT_TRUE(tree.roots().empty());
}

TEST_F(SeriesReducedTreeTest, EraseLinkedFails) {
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.insert(20));
    ASSERT_TRUE(tree.link(1, {10, 20}));
    EXPECT_FALSE(tree.erase(10));
    EXPECT_FALSE(tree.erase(1));
}

TEST_F(SeriesReducedTreeTest, EraseMissingFails) {
    EXPECT_FALSE(tree.erase(99));
}

TEST_F(SeriesReducedTreeTest, UnlinkThenTryReduceOnParent) {
    ASSERT_TRUE(tree.insert(0));
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(2));
    ASSERT_TRUE(tree.link(0, {1, 2}));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.insert(20));
    ASSERT_TRUE(tree.link(1, {10, 20}));
    ASSERT_TRUE(tree.unlink(20));
    EXPECT_THAT(tree.children(0), UnorderedElementsAre(10, 2));
    EXPECT_THROW((void)tree.children(1), std::out_of_range);
}

TEST_F(SeriesReducedTreeTest, ChildrenMissingParentThrows) {
    EXPECT_THROW((void)tree.children(99), std::out_of_range);
}
