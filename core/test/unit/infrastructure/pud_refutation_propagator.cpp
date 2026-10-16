#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_refutation_propagator.hpp"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::InSequence;
using ::testing::NiceMock;

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

struct MockGetParent {
    MOCK_METHOD(pud_node_id, get, (pud_node_id));
};

struct MockCheckRoot {
    MOCK_METHOD(bool, check_root, (pud_node_id));
};

using test_propagator_t = pud_refutation_propagator<
    NiceMock<MockCheckRefuted>,
    NiceMock<MockMarkRefuted>,
    NiceMock<MockGetChildren>,
    NiceMock<MockGetParent>,
    NiceMock<MockCheckRoot>>;

// root -> {a, b}; a -> {a1, a2}; b -> {b1}
struct PudRefutationPropagatorTest : public ::testing::Test {
    NiceMock<MockCheckRefuted> check_refuted;
    NiceMock<MockMarkRefuted> mark_refuted;
    NiceMock<MockGetChildren> children;
    NiceMock<MockGetParent> parents;
    NiceMock<MockCheckRoot> roots;
    test_propagator_t propagator{check_refuted, mark_refuted, children, parents, roots};
    std::unordered_map<pud_node_id, std::vector<pud_node_id>> tree;
    std::unordered_map<pud_node_id, pud_node_id> parent_of;
    std::unordered_set<pud_node_id> refuted;
    std::unordered_set<pud_node_id> root_nodes;
    std::vector<pud_node_id> no_children;
    pud_node_id root = 1;
    pud_node_id a = 2;
    pud_node_id b = 3;
    pud_node_id a1 = 4;
    pud_node_id a2 = 5;
    pud_node_id b1 = 6;

    void link(pud_node_id parent, std::vector<pud_node_id> node_children) {
        for (pud_node_id child : node_children)
            parent_of[child] = parent;
        tree[parent] = std::move(node_children);
    }

    void SetUp() override {
        root_nodes.insert(root);
        link(root, {a, b});
        link(a, {a1, a2});
        link(b, {b1});
        ON_CALL(check_refuted, check_refuted(_)).WillByDefault([&](pud_node_id node) {
            return refuted.contains(node);
        });
        ON_CALL(mark_refuted, mark_refuted(_)).WillByDefault([&](pud_node_id node) {
            refuted.insert(node);
        });
        ON_CALL(children, get(_)).WillByDefault([&](pud_node_id node) -> const std::vector<pud_node_id>& {
            auto found = tree.find(node);
            return found == tree.end() ? no_children : found->second;
        });
        ON_CALL(parents, get(_)).WillByDefault([&](pud_node_id node) {
            return parent_of.at(node);
        });
        ON_CALL(roots, check_root(_)).WillByDefault([&](pud_node_id node) {
            return root_nodes.contains(node);
        });
    }

    std::unordered_set<pud_node_id> nodes(std::vector<pud_node_id> listed) {
        return {listed.begin(), listed.end()};
    }
};

// ---------- downward ----------

TEST_F(PudRefutationPropagatorTest, LeafWithLiveSiblingsRefutesOnlyItself) {
    propagator.refute(a1);
    EXPECT_EQ(refuted, nodes({a1}));
}

TEST_F(PudRefutationPropagatorTest, InteriorNodeRefutesItsWholeSubtree) {
    propagator.refute(a);
    EXPECT_EQ(refuted, nodes({a, a1, a2}));
}

TEST_F(PudRefutationPropagatorTest, RootRefutesTheWholeTree) {
    propagator.refute(root);
    EXPECT_EQ(refuted, nodes({root, a, b, a1, a2, b1}));
}

TEST_F(PudRefutationPropagatorTest, AlreadyRefutedNodeDoesNothing) {
    refuted = nodes({a, a1, a2});
    EXPECT_CALL(mark_refuted, mark_refuted(_)).Times(0);
    EXPECT_CALL(children, get(_)).Times(0);
    EXPECT_CALL(parents, get(_)).Times(0);
    propagator.refute(a);
}

TEST_F(PudRefutationPropagatorTest, AlreadyRefutedChildIsSkippedWithItsSubtree) {
    refuted = nodes({a, a1, a2});
    EXPECT_CALL(mark_refuted, mark_refuted(_)).Times(AnyNumber());
    EXPECT_CALL(mark_refuted, mark_refuted(a)).Times(0);
    EXPECT_CALL(mark_refuted, mark_refuted(a1)).Times(0);
    EXPECT_CALL(mark_refuted, mark_refuted(a2)).Times(0);
    propagator.refute(root);
    EXPECT_EQ(refuted, nodes({root, a, b, a1, a2, b1}));
}

TEST_F(PudRefutationPropagatorTest, EveryNodeIsMarkedExactlyOnce) {
    EXPECT_CALL(mark_refuted, mark_refuted(root));
    EXPECT_CALL(mark_refuted, mark_refuted(a));
    EXPECT_CALL(mark_refuted, mark_refuted(b));
    EXPECT_CALL(mark_refuted, mark_refuted(a1));
    EXPECT_CALL(mark_refuted, mark_refuted(a2));
    EXPECT_CALL(mark_refuted, mark_refuted(b1));
    propagator.refute(a1);
    propagator.refute(a2);
    propagator.refute(b);
}

// ---------- upward ----------

TEST_F(PudRefutationPropagatorTest, LastLiveSiblingRefutesTheParent) {
    propagator.refute(a1);
    propagator.refute(a2);
    EXPECT_EQ(refuted, nodes({a, a1, a2}));
}

TEST_F(PudRefutationPropagatorTest, OnlyChildRefutesItsParent) {
    propagator.refute(b1);
    EXPECT_EQ(refuted, nodes({b, b1}));
}

TEST_F(PudRefutationPropagatorTest, RefutationClimbsWhileEveryChildIsRefuted) {
    propagator.refute(a);
    propagator.refute(b1);
    EXPECT_EQ(refuted, nodes({root, a, b, a1, a2, b1}));
}

TEST_F(PudRefutationPropagatorTest, ClimbStopsAtTheFirstParentWithALiveChild) {
    propagator.refute(a1);
    propagator.refute(a2);
    EXPECT_FALSE(refuted.contains(root));
    EXPECT_FALSE(refuted.contains(b));
}

TEST_F(PudRefutationPropagatorTest, RootStopsTheClimb) {
    EXPECT_CALL(parents, get(_)).Times(AnyNumber());
    EXPECT_CALL(parents, get(root)).Times(0);
    propagator.refute(root);
}

TEST_F(PudRefutationPropagatorTest, ParentIsMarkedAfterItsLastChild) {
    propagator.refute(a1);
    InSequence in_order;
    EXPECT_CALL(mark_refuted, mark_refuted(a2));
    EXPECT_CALL(mark_refuted, mark_refuted(a));
    propagator.refute(a2);
}

TEST_F(PudRefutationPropagatorTest, WholeSubtreeIsMarkedBeforeAnyAncestor) {
    refuted = nodes({b, b1});
    InSequence in_order;
    EXPECT_CALL(mark_refuted, mark_refuted(a));
    EXPECT_CALL(mark_refuted, mark_refuted(a2));
    EXPECT_CALL(mark_refuted, mark_refuted(a1));
    EXPECT_CALL(mark_refuted, mark_refuted(root));
    propagator.refute(a);
}

TEST_F(PudRefutationPropagatorTest, ExpandedNodeWithNoChildrenRefutesItsParentWhenLast) {
    propagator.refute(a1);
    tree[a2] = {};
    propagator.refute(a2);
    EXPECT_TRUE(refuted.contains(a));
}

TEST_F(PudRefutationPropagatorTest, DeepChainCollapsesToTheRootFromItsOnlyLeaf) {
    const pud_node_id depth = 200;
    tree.clear();
    parent_of.clear();
    root_nodes = {100};
    for (pud_node_id level = 0; level < depth; ++level)
        link(100 + level, {100 + level + 1});
    propagator.refute(100 + depth);
    EXPECT_EQ(refuted.size(), depth + 1);
    EXPECT_TRUE(refuted.contains(100));
}

TEST_F(PudRefutationPropagatorTest, WideSiblingSetRefutesTheParentOnlyAtTheLastChild) {
    const pud_node_id width = 100;
    std::vector<pud_node_id> wide_children;
    for (pud_node_id column = 0; column < width; ++column)
        wide_children.push_back(1000 + column);
    link(b1, wide_children);
    for (pud_node_id column = 0; column + 1 < width; ++column) {
        propagator.refute(1000 + column);
        EXPECT_FALSE(refuted.contains(b1));
    }
    propagator.refute(1000 + width - 1);
    EXPECT_TRUE(refuted.contains(b1));
    EXPECT_TRUE(refuted.contains(b));
    EXPECT_FALSE(refuted.contains(root));
}

}  // namespace
