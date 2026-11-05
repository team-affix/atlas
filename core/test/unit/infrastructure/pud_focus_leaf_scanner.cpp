#include <optional>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_focus_leaf_scanner.hpp"
#include "pud_test_node.hpp"
#include "value_objects/pud_frontier_change.hpp"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::UnorderedElementsAre;

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
struct MockTrimCaches {
    MOCK_METHOD(void, trim, ());
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


struct PudFocusLeafScannerTest : public ::testing::Test, StoredTree {
    NiceMock<MockTrimCaches> caches;
    pud_focus_leaf_scanner<
        NiceMock<MockGetChildren>, NiceMock<MockIsNodeRefuted>, NiceMock<MockIsNodeLeaf>, NiceMock<MockTrimCaches>>
        scanner{children, refuted, leaves, caches, pud_test_node(0)};
    void SetUp() override { stub(); }
};

}

TEST_F(PudFocusLeafScannerTest, TheFocusModulesOffspringAreTheFocusTreesUnrefutedLeaves) {
    ON_CALL(refuted, is_refuted(pud_test_node(4))).WillByDefault(Return(true));
    const pud_frontier_change change = scanner.activate_module(k_test_module);
    EXPECT_THAT(change.offspring, UnorderedElementsAre(pud_test_node(2), pud_test_node(3)));
    EXPECT_THAT(change.refuted, IsEmpty());
}

TEST_F(PudFocusLeafScannerTest, ARefutedSubtreeIsNotWalked) {
    ON_CALL(refuted, is_refuted(pud_test_node(1))).WillByDefault(Return(true));
    EXPECT_CALL(children, get_children(_)).Times(AnyNumber());
    EXPECT_CALL(children, get_children(pud_test_node(1))).Times(0);
    EXPECT_THAT(scanner.activate_module(k_test_module).offspring, ElementsAre(pud_test_node(2)));
}

TEST_F(PudFocusLeafScannerTest, AnotherModuleHasNoOffspring) {
    EXPECT_THAT(scanner.activate_module(k_test_module + 1).offspring, IsEmpty());
}

TEST_F(PudFocusLeafScannerTest, ASmallTreeIsWalkedWithoutTrimming) {
    EXPECT_CALL(caches, trim()).Times(0);
    scanner.activate_module(k_test_module);
}

TEST_F(PudFocusLeafScannerTest, ALargeTreeIsWalkedWithTheCachesTrimmedAlongTheWay) {
    child_lists[0].clear();
    for (pud_node_id child = 10; child < 10 + 10000; ++child)
        child_lists[0].push_back(child);
    EXPECT_CALL(caches, trim()).Times(2);
    EXPECT_EQ(scanner.activate_module(k_test_module).offspring.size(), 10000u);
}
