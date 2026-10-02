#include <optional>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_witness_search_head.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

using child_iter = std::vector<const pud_node*>::const_iterator;

struct MockCheckLeaf {
    MOCK_METHOD(bool, check_leaf, (const pud_node*));
};

struct MockGetChildren {
    MOCK_METHOD((const std::vector<const pud_node*>&), get, (const pud_node*));
};

struct MockPropagate {
    MOCK_METHOD(std::optional<int>, propagate, (int, const pud_node*));
};

struct MockCallSite {
    MOCK_METHOD(size_t, get, (const pud_node*));
};

using test_witness_head_t = pud_witness_search_head<
    int,
    child_iter,
    MockCheckLeaf,
    MockGetChildren,
    MockPropagate,
    MockCallSite>;

struct PudWitnessSearchHeadTest : public ::testing::Test {
    NiceMock<MockCheckLeaf> leaves;
    NiceMock<MockGetChildren> children;
    NiceMock<MockPropagate> propagate;
    NiceMock<MockCallSite> call_sites;
    pud_node root{};
    pud_node left{};
    pud_node right{};
    pud_node left_1{};
    pud_node left_2{};
    pud_node deep_a{};
    pud_node deep_b{};
    pud_node deep_c{};
    pud_node deep_leaf{};
    pud_node wide_0{};
    pud_node wide_1{};
    pud_node wide_2{};
    pud_node wide_3{};
    std::unordered_map<const pud_node*, std::vector<const pud_node*>> sequences;

    const std::vector<const pud_node*>& kids(const pud_node* node) {
        return sequences.at(node);
    }

    test_witness_head_t make_head(const pud_node* node, int handle) {
        return test_witness_head_t{
            leaves, children, propagate, call_sites,
            pud_query_position<int>{.handle = handle, .node = node}};
    }

    void SetUp() override {
        ON_CALL(leaves, check_leaf(_)).WillByDefault(Return(false));
        ON_CALL(children, get(_)).WillByDefault([&](const pud_node* node) -> const std::vector<const pud_node*>& {
            return sequences.at(node);
        });
        ON_CALL(propagate, propagate(_, _)).WillByDefault([](int, const pud_node*) {
            return std::optional<int>{1};
        });
    }
};

TEST_F(PudWitnessSearchHeadTest, NoLeafIsReachable) {
    sequences[&root] = {&left};
    sequences[&left] = {};
    EXPECT_CALL(leaves, check_leaf(&root)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(&left)).WillRepeatedly(Return(false));
    EXPECT_CALL(propagate, propagate(_, &left)).WillRepeatedly(Return(std::nullopt));
    auto head = make_head(&root, 0);
    EXPECT_FALSE(head.resume().has_value());
}

TEST_F(PudWitnessSearchHeadTest, FirstChildRefusedAndSiblingIsTheLeaf) {
    sequences[&root] = {&left, &right};
    EXPECT_CALL(leaves, check_leaf(&root)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(&right)).WillRepeatedly(Return(true));
    EXPECT_CALL(propagate, propagate(_, _))
        .WillOnce(Return(std::nullopt))
        .WillOnce(Return(2));
    auto head = make_head(&root, 0);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found, &right);
}

TEST_F(PudWitnessSearchHeadTest, LaterSiblingIsTheLeaf) {
    sequences[&root] = {&left, &right};
    EXPECT_CALL(leaves, check_leaf(&root)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(&right)).WillRepeatedly(Return(true));
    EXPECT_CALL(propagate, propagate(_, &left)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(propagate, propagate(_, &right)).WillOnce(Return(2));
    auto head = make_head(&root, 0);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found, &right);
}

TEST_F(PudWitnessSearchHeadTest, DeadSubtreeReturnsParentsNextChild) {
    sequences[&root] = {&left, &right};
    sequences[&left] = {&left_1, &left_2};
    sequences[&left_1] = {&deep_a, &deep_b};
    EXPECT_CALL(leaves, check_leaf(&left_2)).WillRepeatedly(Return(true));
    EXPECT_CALL(propagate, propagate(_, &left)).WillOnce(Return(2));
    EXPECT_CALL(propagate, propagate(_, &left_1)).WillOnce(Return(3));
    EXPECT_CALL(propagate, propagate(_, &deep_a)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(propagate, propagate(_, &deep_b)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(propagate, propagate(_, &left_2)).WillOnce(Return(4));
    auto head = make_head(&root, 0);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found, &left_2);
}

TEST_F(PudWitnessSearchHeadTest, DeepLeafBeforeShallowSibling) {
    sequences[&root] = {&left, &right};
    sequences[&left] = {&left_1, &left_2};
    sequences[&left_1] = {&deep_a, &deep_leaf};
    EXPECT_CALL(leaves, check_leaf(&deep_leaf)).WillRepeatedly(Return(true));
    EXPECT_CALL(propagate, propagate(_, &left)).WillOnce(Return(2));
    EXPECT_CALL(propagate, propagate(_, &left_1)).WillOnce(Return(3));
    EXPECT_CALL(propagate, propagate(_, &deep_a)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(propagate, propagate(_, &deep_leaf)).WillOnce(Return(5));
    auto head = make_head(&root, 0);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found, &deep_leaf);
}

TEST_F(PudWitnessSearchHeadTest, WideNodeThenDeepLeaf) {
    sequences[&root] = {&wide_0, &wide_1, &wide_2, &wide_3};
    sequences[&wide_3] = {&deep_a};
    sequences[&deep_a] = {&deep_b};
    sequences[&deep_b] = {&deep_leaf};
    EXPECT_CALL(leaves, check_leaf(&deep_leaf)).WillRepeatedly(Return(true));
    EXPECT_CALL(propagate, propagate(_, &wide_0)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(propagate, propagate(_, &wide_1)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(propagate, propagate(_, &wide_2)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(propagate, propagate(_, &wide_3)).WillOnce(Return(2));
    EXPECT_CALL(propagate, propagate(_, &deep_a)).WillOnce(Return(3));
    EXPECT_CALL(propagate, propagate(_, &deep_b)).WillOnce(Return(4));
    EXPECT_CALL(propagate, propagate(_, &deep_leaf)).WillOnce(Return(5));
    auto head = make_head(&root, 0);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found, &deep_leaf);
}

TEST_F(PudWitnessSearchHeadTest, DeepBranchDiesAndOtherBranchLeafIsFound) {
    sequences[&root] = {&left, &right};
    sequences[&left] = {&left_1};
    sequences[&left_1] = {&deep_a};
    sequences[&deep_a] = {&deep_b};
    sequences[&right] = {&deep_leaf};
    EXPECT_CALL(leaves, check_leaf(&deep_leaf)).WillRepeatedly(Return(true));
    EXPECT_CALL(propagate, propagate(_, &left)).WillOnce(Return(2));
    EXPECT_CALL(propagate, propagate(_, &left_1)).WillOnce(Return(3));
    EXPECT_CALL(propagate, propagate(_, &deep_a)).WillOnce(Return(4));
    EXPECT_CALL(propagate, propagate(_, &deep_b)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(propagate, propagate(_, &right)).WillOnce(Return(6));
    EXPECT_CALL(propagate, propagate(_, &deep_leaf)).WillOnce(Return(7));
    auto head = make_head(&root, 0);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found, &deep_leaf);
}

TEST_F(PudWitnessSearchHeadTest, AdvanceRootKeepsRootIterators) {
    sequences[&root] = {&left, &right};
    sequences[&left] = {&deep_leaf};
    EXPECT_CALL(leaves, check_leaf(&deep_leaf)).WillRepeatedly(Return(true));
    EXPECT_CALL(propagate, propagate(_, &left)).WillOnce(Return(2));
    EXPECT_CALL(propagate, propagate(_, &deep_leaf)).WillOnce(Return(3));
    auto head = make_head(&root, 0);
    ASSERT_TRUE(head.resume().has_value());
    auto [root_frame, empty] = head.advance_root();
    EXPECT_FALSE(empty);
    EXPECT_EQ(root_frame.position.node, &root);
    EXPECT_EQ(*root_frame.next_child_it, &right);
    EXPECT_EQ(root_frame.end_child_it, sequences[&root].end());
}

TEST_F(PudWitnessSearchHeadTest, ForkThatCannotEnterDeepestContinuesParentChildren) {
    sequences[&root] = {&left};
    sequences[&left] = {&left_1, &left_2};
    EXPECT_CALL(leaves, check_leaf(&left_1)).WillRepeatedly(Return(true));
    EXPECT_CALL(leaves, check_leaf(&left_2)).WillRepeatedly(Return(true));
    EXPECT_CALL(propagate, propagate(_, &left)).WillOnce(Return(2));
    EXPECT_CALL(propagate, propagate(_, &left_1)).WillOnce(Return(3));
    auto head = make_head(&root, 0);
    ASSERT_EQ(head.resume().value_or(nullptr), &left_1);
    EXPECT_CALL(propagate, propagate(9, &root)).WillOnce(Return(10));
    EXPECT_CALL(propagate, propagate(_, &left)).WillOnce(Return(11));
    EXPECT_CALL(propagate, propagate(_, &left_1)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(propagate, propagate(_, &left_2)).WillOnce(Return(12));
    test_witness_head_t forked{head, 9};
    auto found = forked.resume();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found, &left_2);
}

TEST_F(PudWitnessSearchHeadTest, ForkThatCannotEnterRootHasNoLeaf) {
    EXPECT_CALL(leaves, check_leaf(&root)).WillRepeatedly(Return(true));
    auto head = make_head(&root, 0);
    ASSERT_EQ(head.resume().value_or(nullptr), &root);
    EXPECT_CALL(propagate, propagate(9, &root)).WillOnce(Return(std::nullopt));
    test_witness_head_t forked{head, 9};
    EXPECT_FALSE(forked.resume().has_value());
}

TEST_F(PudWitnessSearchHeadTest, LeafRootIsThatNode) {
    EXPECT_CALL(leaves, check_leaf(&root)).WillRepeatedly(Return(true));
    auto head = make_head(&root, 0);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found, &root);
}

TEST_F(PudWitnessSearchHeadTest, ChainOfFourReachesTheLeaf) {
    sequences[&root] = {&left};
    sequences[&left] = {&left_1};
    sequences[&left_1] = {&deep_leaf};
    EXPECT_CALL(leaves, check_leaf(&deep_leaf)).WillRepeatedly(Return(true));
    EXPECT_CALL(propagate, propagate(_, &left)).WillOnce(Return(2));
    EXPECT_CALL(propagate, propagate(_, &left_1)).WillOnce(Return(3));
    EXPECT_CALL(propagate, propagate(_, &deep_leaf)).WillOnce(Return(4));
    auto head = make_head(&root, 0);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found, &deep_leaf);
}
