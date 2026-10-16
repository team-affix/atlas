#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_live_leaf_finder.hpp"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::UnorderedElementsAre;

namespace {

struct MockCheckRefuted {
    MOCK_METHOD(bool, check_refuted, (pud_node_id));
};

struct MockCheckLeaf {
    MOCK_METHOD(bool, check_leaf, (pud_node_id));
};

struct MockGetChildren {
    MOCK_METHOD((const std::vector<pud_node_id>&), get, (pud_node_id));
};

using test_finder_t = pud_live_leaf_finder<
    NiceMock<MockCheckRefuted>,
    NiceMock<MockCheckLeaf>,
    NiceMock<MockGetChildren>>;

// root -> {a, b}; a -> {a1, a2}; b -> {b1}
struct PudLiveLeafFinderTest : public ::testing::Test {
    NiceMock<MockCheckRefuted> refuted_nodes;
    NiceMock<MockCheckLeaf> leaves;
    NiceMock<MockGetChildren> children;
    test_finder_t finder{refuted_nodes, leaves, children};
    std::unordered_map<pud_node_id, std::vector<pud_node_id>> tree;
    std::unordered_set<pud_node_id> refuted;
    std::vector<pud_node_id> no_children;
    pud_node_id root = 1;
    pud_node_id a = 2;
    pud_node_id b = 3;
    pud_node_id a1 = 4;
    pud_node_id a2 = 5;
    pud_node_id b1 = 6;

    void SetUp() override {
        tree[root] = {a, b};
        tree[a] = {a1, a2};
        tree[b] = {b1};
        ON_CALL(refuted_nodes, check_refuted(_)).WillByDefault([&](pud_node_id node) {
            return refuted.contains(node);
        });
        ON_CALL(leaves, check_leaf(_)).WillByDefault([&](pud_node_id node) {
            return !tree.contains(node);
        });
        ON_CALL(children, get(_)).WillByDefault([&](pud_node_id node) -> const std::vector<pud_node_id>& {
            auto found = tree.find(node);
            return found == tree.end() ? no_children : found->second;
        });
    }
};

TEST_F(PudLiveLeafFinderTest, LeafRootIsItsOwnLiveLeaf) {
    EXPECT_THAT(finder.find_live_leaves(a1), UnorderedElementsAre(a1));
}

TEST_F(PudLiveLeafFinderTest, FindsEveryLeafUnderTheNode) {
    EXPECT_THAT(finder.find_live_leaves(root), UnorderedElementsAre(a1, a2, b1));
}

TEST_F(PudLiveLeafFinderTest, OnlyLooksUnderTheGivenNode) {
    EXPECT_THAT(finder.find_live_leaves(a), UnorderedElementsAre(a1, a2));
}

TEST_F(PudLiveLeafFinderTest, RefutedLeavesAreSkipped) {
    refuted = {a1};
    EXPECT_THAT(finder.find_live_leaves(root), UnorderedElementsAre(a2, b1));
}

TEST_F(PudLiveLeafFinderTest, RefutedInteriorNodeHidesItsSubtree) {
    refuted = {a};
    EXPECT_CALL(children, get(_)).Times(AnyNumber());
    EXPECT_CALL(children, get(a)).Times(0);
    EXPECT_THAT(finder.find_live_leaves(root), UnorderedElementsAre(b1));
}

TEST_F(PudLiveLeafFinderTest, RefutedRootHasNoLiveLeaves) {
    refuted = {root};
    EXPECT_CALL(children, get(_)).Times(0);
    EXPECT_THAT(finder.find_live_leaves(root), IsEmpty());
}

TEST_F(PudLiveLeafFinderTest, ExpandedNodeWithNoChildrenHasNoLiveLeaves) {
    tree[b1] = {};
    EXPECT_THAT(finder.find_live_leaves(b), IsEmpty());
}

TEST_F(PudLiveLeafFinderTest, LeavesAreNotExpanded) {
    EXPECT_CALL(children, get(_)).Times(AnyNumber());
    EXPECT_CALL(children, get(a1)).Times(0);
    EXPECT_CALL(children, get(a2)).Times(0);
    finder.find_live_leaves(a);
}

TEST_F(PudLiveLeafFinderTest, WideAndDeepTreeFindsEveryLeafOnce) {
    const pud_node_id width = 50;
    const pud_node_id depth = 50;
    tree.clear();
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
    std::vector<pud_node_id> found = finder.find_live_leaves(root);
    std::sort(found.begin(), found.end());
    std::sort(expected_leaves.begin(), expected_leaves.end());
    EXPECT_EQ(found, expected_leaves);
}

}  // namespace
