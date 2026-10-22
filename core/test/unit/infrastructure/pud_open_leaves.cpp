#include <set>
#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/pud_open_leaves.hpp"

struct PudOpenLeavesTest : public ::testing::Test {
    pud_open_leaves open_leaves;
    pud_node_id leaf = 3;
    pud_node_id other = 1;
};

TEST_F(PudOpenLeavesTest, NothingIsOpenAtFirst) {
    EXPECT_FALSE(open_leaves.check_leaf_open(leaf));
    EXPECT_TRUE(open_leaves.get().empty());
}

TEST_F(PudOpenLeavesTest, MarkedLeafIsOpen) {
    open_leaves.mark_leaf_open(leaf);
    EXPECT_TRUE(open_leaves.check_leaf_open(leaf));
    EXPECT_FALSE(open_leaves.check_leaf_open(other));
}

TEST_F(PudOpenLeavesTest, ClosedLeafIsNoLongerOpen) {
    open_leaves.mark_leaf_open(leaf);
    open_leaves.mark_leaf_open(other);
    open_leaves.mark_leaf_closed(leaf);
    EXPECT_FALSE(open_leaves.check_leaf_open(leaf));
    EXPECT_EQ(open_leaves.get(), (std::set<pud_node_id>{other}));
}

TEST_F(PudOpenLeavesTest, GetListsOpenLeavesOldestFirst) {
    open_leaves.mark_leaf_open(leaf);
    open_leaves.mark_leaf_open(other);
    EXPECT_EQ(*open_leaves.get().begin(), other);
}

TEST_F(PudOpenLeavesTest, OpeningAnOpenLeafThrows) {
    open_leaves.mark_leaf_open(leaf);
    EXPECT_THROW(open_leaves.mark_leaf_open(leaf), std::logic_error);
}

TEST_F(PudOpenLeavesTest, ClosingALeafThatIsNotOpenThrows) {
    EXPECT_THROW(open_leaves.mark_leaf_closed(leaf), std::logic_error);
    open_leaves.mark_leaf_open(leaf);
    open_leaves.mark_leaf_closed(leaf);
    EXPECT_THROW(open_leaves.mark_leaf_closed(leaf), std::logic_error);
}

TEST_F(PudOpenLeavesTest, ClosedLeafCanBeOpenedAgain) {
    open_leaves.mark_leaf_open(leaf);
    open_leaves.mark_leaf_closed(leaf);
    open_leaves.mark_leaf_open(leaf);
    EXPECT_TRUE(open_leaves.check_leaf_open(leaf));
}
