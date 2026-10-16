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

struct MockDecrementLiveChildCount {
    MOCK_METHOD(size_t, decrement, (pud_node_id));
};

struct MockCheckRoot {
    MOCK_METHOD(bool, check_root, (pud_node_id));
};

struct MockGetParent {
    MOCK_METHOD(pud_node_id, get, (pud_node_id));
};

using test_propagator_t = pud_refutation_propagator<
    NiceMock<MockDecrementLiveChildCount>,
    NiceMock<MockCheckRoot>,
    NiceMock<MockGetParent>>;

// root -> {a, b}; a -> {a1, a2}; b -> {b1}
struct PudRefutationPropagatorTest : public ::testing::Test {
    NiceMock<MockDecrementLiveChildCount> live_child_counts;
    NiceMock<MockCheckRoot> roots;
    NiceMock<MockGetParent> parents;
    test_propagator_t propagator{live_child_counts, roots, parents};
    std::unordered_map<pud_node_id, size_t> count_of;
    std::unordered_map<pud_node_id, pud_node_id> parent_of;
    std::unordered_set<pud_node_id> root_nodes;
    pud_node_id root = 1;
    pud_node_id a = 2;
    pud_node_id b = 3;
    pud_node_id a1 = 4;
    pud_node_id a2 = 5;
    pud_node_id b1 = 6;

    void link(pud_node_id parent, std::vector<pud_node_id> node_children) {
        count_of[parent] = node_children.size();
        for (pud_node_id child : node_children) {
            parent_of[child] = parent;
            count_of.try_emplace(child, 1);
        }
    }

    void SetUp() override {
        root_nodes.insert(root);
        link(root, {a, b});
        link(a, {a1, a2});
        link(b, {b1});
        ON_CALL(live_child_counts, decrement(_)).WillByDefault([&](pud_node_id node) {
            return --count_of.at(node);
        });
        ON_CALL(roots, check_root(_)).WillByDefault([&](pud_node_id node) {
            return root_nodes.contains(node);
        });
        ON_CALL(parents, get(_)).WillByDefault([&](pud_node_id node) {
            return parent_of.at(node);
        });
    }

    bool refuted(pud_node_id node) {
        return count_of.at(node) == 0;
    }
};

TEST_F(PudRefutationPropagatorTest, LeafWithALiveSiblingRefutesOnlyItself) {
    propagator.refute(a1);
    EXPECT_TRUE(refuted(a1));
    EXPECT_EQ(count_of.at(a), 1u);
    EXPECT_EQ(count_of.at(root), 2u);
}

TEST_F(PudRefutationPropagatorTest, LastLiveSiblingRefutesTheParent) {
    propagator.refute(a1);
    propagator.refute(a2);
    EXPECT_TRUE(refuted(a));
    EXPECT_EQ(count_of.at(root), 1u);
}

TEST_F(PudRefutationPropagatorTest, OnlyChildRefutesItsParent) {
    propagator.refute(b1);
    EXPECT_TRUE(refuted(b));
    EXPECT_EQ(count_of.at(root), 1u);
}

TEST_F(PudRefutationPropagatorTest, RefutationClimbsToTheRootOnceEveryLeafIsRefuted) {
    propagator.refute(a1);
    propagator.refute(a2);
    propagator.refute(b1);
    EXPECT_TRUE(refuted(root));
}

TEST_F(PudRefutationPropagatorTest, ClimbStopsAtTheFirstAncestorWithALiveChild) {
    EXPECT_CALL(parents, get(_)).Times(AnyNumber());
    EXPECT_CALL(parents, get(root)).Times(0);
    propagator.refute(a1);
    propagator.refute(a2);
    EXPECT_FALSE(refuted(root));
}

TEST_F(PudRefutationPropagatorTest, LeafWithALiveSiblingNeverLooksAtItsParentsParent) {
    EXPECT_CALL(parents, get(_)).Times(AnyNumber());
    EXPECT_CALL(parents, get(a)).Times(0);
    propagator.refute(a1);
}

TEST_F(PudRefutationPropagatorTest, RefutedRootStopsTheClimb) {
    count_of = {{root, 1}};
    EXPECT_CALL(parents, get(_)).Times(0);
    propagator.refute(root);
    EXPECT_TRUE(refuted(root));
}

TEST_F(PudRefutationPropagatorTest, EachLevelIsDecrementedOnceInClimbingOrder) {
    propagator.refute(a1);
    propagator.refute(b1);
    InSequence in_order;
    EXPECT_CALL(live_child_counts, decrement(a2));
    EXPECT_CALL(live_child_counts, decrement(a));
    EXPECT_CALL(live_child_counts, decrement(root));
    propagator.refute(a2);
}

TEST_F(PudRefutationPropagatorTest, DeepChainCollapsesToTheRootFromItsOnlyLeaf) {
    const pud_node_id depth = 200;
    count_of.clear();
    parent_of.clear();
    root_nodes = {100};
    for (pud_node_id level = 0; level < depth; ++level)
        link(100 + level, {100 + level + 1});
    propagator.refute(100 + depth);
    for (pud_node_id level = 0; level <= depth; ++level)
        EXPECT_TRUE(refuted(100 + level));
}

TEST_F(PudRefutationPropagatorTest, WideSiblingSetRefutesTheParentOnlyAtTheLastChild) {
    const pud_node_id width = 100;
    std::vector<pud_node_id> wide_children;
    for (pud_node_id column = 0; column < width; ++column)
        wide_children.push_back(1000 + column);
    link(b1, wide_children);
    for (pud_node_id column = 0; column + 1 < width; ++column) {
        propagator.refute(1000 + column);
        EXPECT_FALSE(refuted(b1));
    }
    propagator.refute(1000 + width - 1);
    EXPECT_TRUE(refuted(b1));
    EXPECT_TRUE(refuted(b));
    EXPECT_FALSE(refuted(root));
}

}  // namespace
