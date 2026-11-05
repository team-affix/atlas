#include <optional>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_child_scan_refutation_propagator.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetParent {
    MOCK_METHOD(std::optional<pud_global_node_id>, get_parent, (pud_global_node_id), (const));
};
struct MockGetChildren {
    MOCK_METHOD(const std::vector<pud_node_id>&, get_children, (pud_global_node_id), (const));
};
struct MockIsNodeRefuted {
    MOCK_METHOD(bool, is_refuted, (pud_global_node_id), (const));
};
struct MockIsNodeLeaf {
    MOCK_METHOD(bool, is_leaf, (pud_global_node_id), (const));
};
struct MockMarkNodeRefuted {
    MOCK_METHOD(void, mark_refuted, (pud_global_node_id));
};

// The stored tree: axiom 0 has children 1, 2; node 1 has children 3, 4; nodes 2, 3, 4 are leaves.
// Axiom 5 of the same module and node 9 of another module lie outside the focus 0.
struct StoredTree {
    NiceMock<MockGetParent> parents;
    NiceMock<MockGetChildren> children;
    NiceMock<MockIsNodeRefuted> refuted;
    NiceMock<MockIsNodeLeaf> leaves;
    std::unordered_map<pud_node_id, std::vector<pud_node_id>> child_lists{{0, {1, 2}}, {1, {3, 4}}};
    std::vector<pud_node_id> no_children{};

    void stub() {
        ON_CALL(parents, get_parent(_)).WillByDefault(Return(std::nullopt));
        ON_CALL(parents, get_parent(pud_test_node(1))).WillByDefault(Return(pud_test_node(0)));
        ON_CALL(parents, get_parent(pud_test_node(2))).WillByDefault(Return(pud_test_node(0)));
        ON_CALL(parents, get_parent(pud_test_node(3))).WillByDefault(Return(pud_test_node(1)));
        ON_CALL(parents, get_parent(pud_test_node(4))).WillByDefault(Return(pud_test_node(1)));
        ON_CALL(children, get_children(_)).WillByDefault(ReturnRef(no_children));
        for (const auto& [node, listed] : child_lists)
            ON_CALL(children, get_children(pud_test_node(node))).WillByDefault(ReturnRef(child_lists.at(node)));
        ON_CALL(refuted, is_refuted(_)).WillByDefault(Return(false));
        ON_CALL(leaves, is_leaf(_)).WillByDefault(Return(true));
        ON_CALL(leaves, is_leaf(pud_test_node(0))).WillByDefault(Return(false));
        ON_CALL(leaves, is_leaf(pud_test_node(1))).WillByDefault(Return(false));
    }
};


struct PudChildScanRefutationPropagatorTest : public ::testing::Test, StoredTree {
    NiceMock<MockMarkNodeRefuted> mark;
    pud_child_scan_refutation_propagator<
        NiceMock<MockGetParent>, NiceMock<MockGetChildren>, NiceMock<MockIsNodeRefuted>, NiceMock<MockMarkNodeRefuted>>
        propagator{parents, children, refuted, mark};
    void SetUp() override { stub(); }
};

}

TEST_F(PudChildScanRefutationPropagatorTest, AParentWithALiveChildStaysUnrefuted) {
    ON_CALL(refuted, is_refuted(pud_test_node(3))).WillByDefault(Return(true));
    EXPECT_CALL(mark, mark_refuted(_)).Times(0);
    propagator.refute(pud_test_node(3));
}

TEST_F(PudChildScanRefutationPropagatorTest, AParentWhoseChildrenAreAllRefutedIsRefutedAndSoOnUpward) {
    ON_CALL(refuted, is_refuted(pud_test_node(3))).WillByDefault(Return(true));
    ON_CALL(refuted, is_refuted(pud_test_node(4))).WillByDefault(Return(true));
    ON_CALL(refuted, is_refuted(pud_test_node(2))).WillByDefault(Return(true));
    // Node 1 reads as refuted to its parent's scan, as it will be once marked.
    ON_CALL(refuted, is_refuted(pud_test_node(1))).WillByDefault(Return(true));
    InSequence in_order;
    EXPECT_CALL(mark, mark_refuted(pud_test_node(1)));
    EXPECT_CALL(mark, mark_refuted(pud_test_node(0)));
    propagator.refute(pud_test_node(4));
}

TEST_F(PudChildScanRefutationPropagatorTest, ARootHasNoParentToRefute) {
    EXPECT_CALL(mark, mark_refuted(_)).Times(0);
    propagator.refute(pud_test_node(0));
}
