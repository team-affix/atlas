#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_children.hpp"
#include "infrastructure/pud_witness_search_head.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

using child_iter = std::vector<pud_node_id>::const_iterator;

struct descent_t {
    pud_node_id node;
    bool operator==(const descent_t&) const = default;
};

struct MockCheckLeaf {
    MOCK_METHOD(bool, check_leaf, (pud_node_id));
};

struct MockDescend {
    MOCK_METHOD(std::optional<descent_t>, descend, (descent_t, pud_node_id));
};

struct MockCallSite {
    MOCK_METHOD(size_t, get, (pud_node_id));
};

using test_head_t = pud_witness_search_head<
    descent_t, child_iter, MockCheckLeaf, pud_children, MockDescend, MockCallSite>;

struct PudWitnessChildrenIntegrationTest : public ::testing::Test {
    NiceMock<MockCheckLeaf> leaves;
    pud_children children;
    NiceMock<MockDescend> descend;
    NiceMock<MockCallSite> call_sites;
    pud_node_id root       = 1;
    pud_node_id left       = 2;
    pud_node_id right      = 3;
    pud_node_id left_dead  = 4;
    pud_node_id left_leaf  = 5;
    pud_node_id dead_child = 6;
    pud_node_id depth1     = 7;
    pud_node_id depth2     = 8;
    pud_node_id depth3     = 9;
    pud_node_id extra      = 10;

    test_head_t make_head(pud_node_id node) {
        return test_head_t{
            leaves, children, descend, call_sites,
            descent_t{node}};
    }

    void SetUp() override {
        ON_CALL(leaves, check_leaf(_)).WillByDefault(Return(false));
        ON_CALL(descend, descend(_, _)).WillByDefault([](descent_t, pud_node_id child) {
            return std::optional<descent_t>{descent_t{child}};
        });
    }
};

TEST_F(PudWitnessChildrenIntegrationTest, DescendedChildStaysAfterOtherNodesAreStored) {
    children.store(root, {left, right});
    EXPECT_CALL(leaves, check_leaf(root)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(left)).WillRepeatedly(Return(true));
    auto head = make_head(root);
    ASSERT_EQ(head.resume().value_or(pud_node_id{0}), left);
    children.store(extra, {depth1});
    auto result = head.advance();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->root_descent.node, left);
    EXPECT_NE(result->root_next_sibling_it, result->root_end_sibling_it);
    EXPECT_EQ(*result->root_next_sibling_it, right);
}

TEST_F(PudWitnessChildrenIntegrationTest, EveryChildFailsToPropagate) {
    children.store(root, {left, right});
    EXPECT_CALL(descend, descend(_, _)).WillRepeatedly(Return(std::nullopt));
    auto head = make_head(root);
    EXPECT_FALSE(head.resume().has_value());
}

TEST_F(PudWitnessChildrenIntegrationTest, DeadSubtreeReturnsParentsNextChildNotTheUncle) {
    children.store(root, {left, right});
    children.store(left, {left_dead, left_leaf});
    children.store(left_dead, {dead_child});
    EXPECT_CALL(leaves, check_leaf(root)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(left)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(left_dead)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(left_leaf)).WillRepeatedly(Return(true));
    EXPECT_CALL(leaves, check_leaf(right)).WillRepeatedly(Return(true));
    EXPECT_CALL(descend, descend(_, left)).WillOnce(Return(std::optional<descent_t>{descent_t{left}}));
    EXPECT_CALL(descend, descend(_, left_dead)).WillOnce(Return(std::optional<descent_t>{descent_t{left_dead}}));
    EXPECT_CALL(descend, descend(_, dead_child)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(descend, descend(_, left_leaf)).WillOnce(Return(std::optional<descent_t>{descent_t{left_leaf}}));
    auto head = make_head(root);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found, left_leaf);
}

TEST_F(PudWitnessChildrenIntegrationTest, BranchOfDepthFourReachesTheLeaf) {
    children.store(root, {depth1});
    children.store(depth1, {depth2});
    children.store(depth2, {depth3});
    EXPECT_CALL(leaves, check_leaf(root)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(depth1)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(depth2)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(depth3)).WillRepeatedly(Return(true));
    auto head = make_head(root);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found, depth3);
}
