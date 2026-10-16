#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_refuted_node_checker.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::StrictMock;

namespace {

struct MockCheckLeaf {
    MOCK_METHOD(bool, check_leaf, (pud_node_id));
};

struct MockCheckLeafRefuted {
    MOCK_METHOD(bool, check_refuted, (pud_node_id));
};

struct MockGetLiveChildCount {
    MOCK_METHOD(size_t, get, (pud_node_id));
};

using test_checker_t = pud_refuted_node_checker<
    StrictMock<MockCheckLeaf>,
    StrictMock<MockCheckLeafRefuted>,
    StrictMock<MockGetLiveChildCount>>;

struct PudRefutedNodeCheckerTest : public ::testing::Test {
    StrictMock<MockCheckLeaf> leaves;
    StrictMock<MockCheckLeafRefuted> refuted_leaves;
    StrictMock<MockGetLiveChildCount> live_child_counts;
    test_checker_t checker{leaves, refuted_leaves, live_child_counts};
    pud_node_id node = 7;
};

TEST_F(PudRefutedNodeCheckerTest, LiveLeafIsNotRefuted) {
    EXPECT_CALL(leaves, check_leaf(node)).WillOnce(Return(true));
    EXPECT_CALL(refuted_leaves, check_refuted(node)).WillOnce(Return(false));
    EXPECT_FALSE(checker.check_refuted(node));
}

TEST_F(PudRefutedNodeCheckerTest, RefutedLeafIsRefuted) {
    EXPECT_CALL(leaves, check_leaf(node)).WillOnce(Return(true));
    EXPECT_CALL(refuted_leaves, check_refuted(node)).WillOnce(Return(true));
    EXPECT_TRUE(checker.check_refuted(node));
}

TEST_F(PudRefutedNodeCheckerTest, LeafNeverLooksAtLiveChildCounts) {
    EXPECT_CALL(leaves, check_leaf(node)).WillOnce(Return(true));
    EXPECT_CALL(refuted_leaves, check_refuted(node)).WillOnce(Return(false));
    EXPECT_CALL(live_child_counts, get(_)).Times(0);
    checker.check_refuted(node);
}

TEST_F(PudRefutedNodeCheckerTest, InteriorNodeWithNoLiveChildrenIsRefuted) {
    EXPECT_CALL(leaves, check_leaf(node)).WillOnce(Return(false));
    EXPECT_CALL(live_child_counts, get(node)).WillOnce(Return(0));
    EXPECT_TRUE(checker.check_refuted(node));
}

TEST_F(PudRefutedNodeCheckerTest, InteriorNodeWithLiveChildrenIsNotRefuted) {
    EXPECT_CALL(leaves, check_leaf(node)).WillOnce(Return(false));
    EXPECT_CALL(live_child_counts, get(node)).WillOnce(Return(3));
    EXPECT_FALSE(checker.check_refuted(node));
}

TEST_F(PudRefutedNodeCheckerTest, InteriorNodeNeverLooksAtRefutedLeaves) {
    EXPECT_CALL(leaves, check_leaf(node)).WillOnce(Return(false));
    EXPECT_CALL(live_child_counts, get(node)).WillOnce(Return(1));
    EXPECT_CALL(refuted_leaves, check_refuted(_)).Times(0);
    checker.check_refuted(node);
}

}  // namespace
