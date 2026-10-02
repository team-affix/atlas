#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_children.hpp"
#include "infrastructure/pud_witness_search_head.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

using child_iter = std::vector<const pud_node*>::const_iterator;

struct MockCheckLeaf {
    MOCK_METHOD(bool, check_leaf, (const pud_node*));
};

struct MockPropagate {
    MOCK_METHOD(std::optional<int>, propagate, (int, const pud_node*));
};

struct MockCallSite {
    MOCK_METHOD(size_t, get, (const pud_node*));
};

using test_head_t = pud_witness_search_head<
    int, child_iter, MockCheckLeaf, pud_children, MockPropagate, MockCallSite>;

struct PudWitnessChildrenIntegrationTest : public ::testing::Test {
    NiceMock<MockCheckLeaf> leaves;
    pud_children children;
    NiceMock<MockPropagate> propagate;
    NiceMock<MockCallSite> call_sites;
    pud_node root{};
    pud_node left{};
    pud_node right{};
    pud_node left_dead{};
    pud_node left_leaf{};
    pud_node dead_child{};
    pud_node depth1{};
    pud_node depth2{};
    pud_node depth3{};
    pud_node extra{};

    test_head_t make_head(const pud_node* node) {
        return test_head_t{
            leaves, children, propagate, call_sites,
            pud_query_position<int>{.handle = 1, .node = node}};
    }

    void SetUp() override {
        ON_CALL(leaves, check_leaf(_)).WillByDefault(Return(false));
        ON_CALL(propagate, propagate(_, _)).WillByDefault(Return(2));
    }
};

TEST_F(PudWitnessChildrenIntegrationTest, DescendedChildStaysAfterOtherNodesAreStored) {
    children.store(&root, {&left, &right});
    EXPECT_CALL(leaves, check_leaf(&root)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(&left)).WillRepeatedly(Return(true));
    auto head = make_head(&root);
    ASSERT_EQ(head.resume().value_or(nullptr), &left);
    children.store(&extra, {&depth1});
    auto [root_frame, empty] = head.advance_root();
    ASSERT_NE(root_frame.next_child_it, root_frame.end_child_it);
    EXPECT_EQ(*root_frame.next_child_it, &right);
    EXPECT_EQ(root_frame.position.node, &root);
    EXPECT_FALSE(empty);
}

TEST_F(PudWitnessChildrenIntegrationTest, EveryChildFailsToPropagate) {
    children.store(&root, {&left, &right});
    EXPECT_CALL(propagate, propagate(_, _)).WillRepeatedly(Return(std::nullopt));
    auto head = make_head(&root);
    EXPECT_FALSE(head.resume().has_value());
}

TEST_F(PudWitnessChildrenIntegrationTest, DeadSubtreeReturnsParentsNextChildNotTheUncle) {
    children.store(&root, {&left, &right});
    children.store(&left, {&left_dead, &left_leaf});
    children.store(&left_dead, {&dead_child});
    EXPECT_CALL(leaves, check_leaf(&root)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(&left)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(&left_dead)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(&left_leaf)).WillRepeatedly(Return(true));
    EXPECT_CALL(leaves, check_leaf(&right)).WillRepeatedly(Return(true));
    EXPECT_CALL(propagate, propagate(_, &left)).WillOnce(Return(2));
    EXPECT_CALL(propagate, propagate(_, &left_dead)).WillOnce(Return(2));
    EXPECT_CALL(propagate, propagate(_, &dead_child)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(propagate, propagate(_, &left_leaf)).WillOnce(Return(2));
    auto head = make_head(&root);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found, &left_leaf);
}

TEST_F(PudWitnessChildrenIntegrationTest, BranchOfDepthFourReachesTheLeaf) {
    children.store(&root, {&depth1});
    children.store(&depth1, {&depth2});
    children.store(&depth2, {&depth3});
    EXPECT_CALL(leaves, check_leaf(&root)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(&depth1)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(&depth2)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(&depth3)).WillRepeatedly(Return(true));
    auto head = make_head(&root);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found, &depth3);
}
