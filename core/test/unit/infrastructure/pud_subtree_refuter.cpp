#include <algorithm>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_subtree_refuter.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::UnorderedElementsAre;

namespace {

struct MockCheckRefuted {
    MOCK_METHOD(bool, check_refuted, (pud_node_id));
};

struct MockMarkRefuted {
    MOCK_METHOD(void, mark_refuted, (pud_node_id));
};

struct MockGetChildren {
    MOCK_METHOD((const std::vector<pud_node_id>&), get, (pud_node_id));
};

struct MockCheckLeaf {
    MOCK_METHOD(bool, check_leaf, (pud_node_id));
};

using test_refuter_t = pud_subtree_refuter<
    NiceMock<MockCheckRefuted>,
    NiceMock<MockMarkRefuted>,
    NiceMock<MockGetChildren>,
    NiceMock<MockCheckLeaf>>;

struct PudSubtreeRefuterTest : public ::testing::Test {
    NiceMock<MockCheckRefuted> check_refuted;
    NiceMock<MockMarkRefuted> mark_refuted;
    NiceMock<MockGetChildren> children;
    NiceMock<MockCheckLeaf> leaves;
    test_refuter_t refuter{check_refuted, mark_refuted, children, leaves};
    std::unordered_map<pud_node_id, std::vector<pud_node_id>> tree;
    std::vector<pud_node_id> no_children;
    pud_node_id root = 1;
    pud_node_id left = 2;
    pud_node_id right = 3;
    pud_node_id left_1 = 4;
    pud_node_id left_2 = 5;

    void SetUp() override {
        ON_CALL(children, get(_)).WillByDefault([&](pud_node_id node) -> const std::vector<pud_node_id>& {
            auto found = tree.find(node);
            return found == tree.end() ? no_children : found->second;
        });
        ON_CALL(leaves, check_leaf(_)).WillByDefault([&](pud_node_id node) {
            return !tree.contains(node);
        });
    }
};

TEST_F(PudSubtreeRefuterTest, LeafRootRefutesOnlyItself) {
    EXPECT_CALL(mark_refuted, mark_refuted(root));
    EXPECT_THAT(refuter.refute(root), UnorderedElementsAre(root));
}

TEST_F(PudSubtreeRefuterTest, EveryNodeInTheSubtreeIsMarked) {
    tree[root] = {left, right};
    tree[left] = {left_1, left_2};
    EXPECT_CALL(mark_refuted, mark_refuted(root));
    EXPECT_CALL(mark_refuted, mark_refuted(left));
    EXPECT_CALL(mark_refuted, mark_refuted(right));
    EXPECT_CALL(mark_refuted, mark_refuted(left_1));
    EXPECT_CALL(mark_refuted, mark_refuted(left_2));
    refuter.refute(root);
}

TEST_F(PudSubtreeRefuterTest, ReturnsExactlyTheSubtreesLeaves) {
    tree[root] = {left, right};
    tree[left] = {left_1, left_2};
    EXPECT_THAT(refuter.refute(root), UnorderedElementsAre(right, left_1, left_2));
}

TEST_F(PudSubtreeRefuterTest, OnlyTheGivenSubtreeIsTouched) {
    tree[root] = {left, right};
    tree[left] = {left_1, left_2};
    EXPECT_CALL(mark_refuted, mark_refuted(_)).Times(::testing::AnyNumber());
    EXPECT_CALL(mark_refuted, mark_refuted(root)).Times(0);
    EXPECT_CALL(mark_refuted, mark_refuted(right)).Times(0);
    EXPECT_THAT(refuter.refute(left), UnorderedElementsAre(left_1, left_2));
}

TEST_F(PudSubtreeRefuterTest, AlreadyRefutedRootDoesNothing) {
    tree[root] = {left, right};
    ON_CALL(check_refuted, check_refuted(root)).WillByDefault(Return(true));
    EXPECT_CALL(mark_refuted, mark_refuted(_)).Times(0);
    EXPECT_CALL(children, get(_)).Times(0);
    EXPECT_TRUE(refuter.refute(root).empty());
}

TEST_F(PudSubtreeRefuterTest, AlreadyRefutedChildIsSkippedWithItsSubtree) {
    tree[root] = {left, right};
    tree[left] = {left_1, left_2};
    ON_CALL(check_refuted, check_refuted(left)).WillByDefault(Return(true));
    EXPECT_CALL(children, get(_)).Times(::testing::AnyNumber());
    EXPECT_CALL(mark_refuted, mark_refuted(root));
    EXPECT_CALL(mark_refuted, mark_refuted(right));
    EXPECT_CALL(mark_refuted, mark_refuted(left)).Times(0);
    EXPECT_CALL(mark_refuted, mark_refuted(left_1)).Times(0);
    EXPECT_CALL(mark_refuted, mark_refuted(left_2)).Times(0);
    EXPECT_CALL(children, get(left)).Times(0);
    EXPECT_THAT(refuter.refute(root), UnorderedElementsAre(right));
}

TEST_F(PudSubtreeRefuterTest, LeavesAreNotExpanded) {
    EXPECT_CALL(children, get(root)).Times(0);
    refuter.refute(root);
}

TEST_F(PudSubtreeRefuterTest, EachNodeIsCheckedBeforeItIsMarked) {
    InSequence in_order;
    EXPECT_CALL(check_refuted, check_refuted(root)).WillOnce(Return(false));
    EXPECT_CALL(mark_refuted, mark_refuted(root));
    refuter.refute(root);
}

TEST_F(PudSubtreeRefuterTest, WideAndDeepSubtreeMarksEveryNodeOnce) {
    const pud_node_id width = 50;
    const pud_node_id depth = 50;
    std::vector<pud_node_id> expected_leaves;
    pud_node_id next_node = 100;
    pud_node_id spine = root;
    for (pud_node_id level = 0; level < depth; ++level) {
        std::vector<pud_node_id> level_children;
        for (pud_node_id column = 0; column < width; ++column)
            level_children.push_back(next_node++);
        tree[spine] = level_children;
        expected_leaves.insert(expected_leaves.end(), level_children.begin() + 1, level_children.end());
        spine = level_children.front();
    }
    expected_leaves.push_back(spine);
    EXPECT_CALL(mark_refuted, mark_refuted(_)).Times(1 + width * depth);
    std::vector<pud_node_id> refuted_leaves = refuter.refute(root);
    std::sort(refuted_leaves.begin(), refuted_leaves.end());
    std::sort(expected_leaves.begin(), expected_leaves.end());
    EXPECT_EQ(refuted_leaves, expected_leaves);
}

}  // namespace
