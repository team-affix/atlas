// dbuct_series_reduced_tree journals every container mutation on the trail, so a
// link()+reduction cascade performed inside a pushed frame is undone exactly when
// the frame pops. These tests assert the forward reduction behaviour matches
// series_reduced_tree and that a pop reconstructs the pre-link topology.

#include <gtest/gtest.h>
#include <set>
#include <unordered_set>
#include "infrastructure/dbuct_series_reduced_tree.hpp"
#include "infrastructure/trail.hpp"

struct DbuctSeriesReducedTreeTest : public ::testing::Test {
protected:
    trail t;
    dbuct_series_reduced_tree<int> tree{t};

    void SetUp() override {
        tree.insert(1);
        tree.insert(2);
        tree.insert(3);
        tree.link(1, std::set<int>{2, 3});
    }
};

TEST_F(DbuctSeriesReducedTreeTest, BranchStructureAfterLink) {
    EXPECT_EQ(tree.roots(), (std::unordered_set<int>{1}));
    EXPECT_EQ(tree.leaves(), (std::unordered_set<int>{2, 3}));
    EXPECT_EQ(tree.children(1), (std::set<int>{2, 3}));
}

TEST_F(DbuctSeriesReducedTreeTest, NullaryThenUnaryCascadeReducesTree) {
    // Removing goal 2 leaves goal 1 with a single child (3), which series-reduces
    // goal 1 away, promoting goal 3 to the root/leaf.
    tree.link(2, std::set<int>{});
    EXPECT_EQ(tree.roots(), (std::unordered_set<int>{3}));
    EXPECT_EQ(tree.leaves(), (std::unordered_set<int>{3}));
}

TEST_F(DbuctSeriesReducedTreeTest, PopRestoresPreCascadeTopology) {
    t.push();
    tree.link(2, std::set<int>{});
    ASSERT_EQ(tree.roots(), (std::unordered_set<int>{3}));
    t.pop();
    EXPECT_EQ(tree.roots(), (std::unordered_set<int>{1}));
    EXPECT_EQ(tree.leaves(), (std::unordered_set<int>{2, 3}));
    EXPECT_EQ(tree.children(1), (std::set<int>{2, 3}));
}

TEST_F(DbuctSeriesReducedTreeTest, PopRestoresAfterAdditionalLink) {
    t.push();
    tree.insert(4);
    tree.insert(5);
    tree.link(2, std::set<int>{4, 5});
    ASSERT_EQ(tree.leaves(), (std::unordered_set<int>{3, 4, 5}));
    t.pop();
    EXPECT_EQ(tree.roots(), (std::unordered_set<int>{1}));
    EXPECT_EQ(tree.leaves(), (std::unordered_set<int>{2, 3}));
    EXPECT_EQ(tree.children(1), (std::set<int>{2, 3}));
}
