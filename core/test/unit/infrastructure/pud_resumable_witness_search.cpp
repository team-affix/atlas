#include <optional>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_resumable_witness_search.hpp"
#include "pud_test_node.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_replay.hpp"
#include "value_objects/pud_witness_advance.hpp"
#include "value_objects/pud_witness_search_stack.hpp"

using ::testing::_;
using ::testing::Field;
using ::testing::Invoke;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Optional;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockDescendReplay {
    MOCK_METHOD(std::optional<pud_replay>, descend, (const pud_replay&, pud_global_node_id));
};
struct MockIsNodeLeaf {
    MOCK_METHOD(bool, is_leaf, (pud_global_node_id), (const));
};
struct MockIsNodeRefuted {
    MOCK_METHOD(bool, is_refuted, (pud_global_node_id), (const));
};
struct MockGetChildren {
    MOCK_METHOD(const std::vector<pud_node_id>&, get_children, (pud_global_node_id));
};

std::optional<pud_replay> pud_enter_child(const pud_replay& parent, pud_global_node_id child) {
    return pud_replay{.node = child, .frame_offset = parent.frame_offset, .ubs_state = parent.ubs_state};
}

using test_witness_search_t = pud_resumable_witness_search<
    NiceMock<MockDescendReplay>, NiceMock<MockIsNodeLeaf>, NiceMock<MockIsNodeRefuted>, NiceMock<MockGetChildren>>;

struct PudResumableWitnessSearchTest : public ::testing::Test {
    NiceMock<MockDescendReplay> descend;
    NiceMock<MockIsNodeLeaf> leaves;
    NiceMock<MockIsNodeRefuted> refuted;
    NiceMock<MockGetChildren> children;
    std::unordered_map<pud_node_id, std::vector<pud_node_id>> child_lists;
    test_witness_search_t search{descend, leaves, refuted, children};
    pud_replay root{.node = pud_test_node(0), .frame_offset = 0u, .ubs_state = {}};
    pud_witness_search_stack stack{.frames = {}};

    void SetUp() override {
        ON_CALL(descend, descend(_, _)).WillByDefault(Invoke(&pud_enter_child));
        ON_CALL(leaves, is_leaf(_)).WillByDefault(Return(false));
        ON_CALL(refuted, is_refuted(_)).WillByDefault(Return(false));
    }

    void interior(pud_node_id node, std::vector<pud_node_id> node_children) {
        child_lists[node] = std::move(node_children);
        ON_CALL(children, get_children(pud_test_node(node))).WillByDefault(ReturnRef(child_lists[node]));
    }
    void leaf(pud_node_id node) {
        interior(node, {});
        ON_CALL(leaves, is_leaf(pud_test_node(node))).WillByDefault(Return(true));
    }
};

}

TEST_F(PudResumableWitnessSearchTest, ALeafRootIsItsOwnWitnessAndLeavesTheStackEmpty) {
    leaf(0);
    EXPECT_THAT(search.resume(root, stack), Optional(pud_test_node(0)));
    EXPECT_THAT(stack.frames, IsEmpty());
}

TEST_F(PudResumableWitnessSearchTest, ARefutedRootHasNoWitness) {
    leaf(0);
    ON_CALL(refuted, is_refuted(pud_test_node(0))).WillByDefault(Return(true));
    EXPECT_FALSE(search.resume(root, stack).has_value());
}

TEST_F(PudResumableWitnessSearchTest, TheWitnessIsTheFirstLeafDepthFirstInChildOrder) {
    interior(0, {1, 2});
    interior(1, {3});
    leaf(3);
    leaf(2);
    EXPECT_THAT(search.resume(root, stack), Optional(pud_test_node(3)));
}

TEST_F(PudResumableWitnessSearchTest, BlockedAndRefutedChildrenAreSkipped) {
    interior(0, {1, 2, 3});
    leaf(1);
    ON_CALL(descend, descend(_, pud_test_node(1))).WillByDefault(Return(std::nullopt));
    leaf(2);
    ON_CALL(refuted, is_refuted(pud_test_node(2))).WillByDefault(Return(true));
    leaf(3);
    EXPECT_THAT(search.resume(root, stack), Optional(pud_test_node(3)));
}

TEST_F(PudResumableWitnessSearchTest, ChildrenAreSteppedIntoInTheirParentsModule) {
    interior(0, {1});
    leaf(1);
    EXPECT_CALL(
        descend,
        descend(Field(&pud_replay::node, pud_test_node(0)), pud_global_node_id{.module = k_test_module, .node = 1}));
    search.resume(root, stack);
}

TEST_F(PudResumableWitnessSearchTest, ASubtreeWithoutLeavesHasNoWitnessAndEmptiesTheStack) {
    interior(0, {1, 2});
    interior(1, {});
    leaf(2);
    ON_CALL(descend, descend(_, pud_test_node(2))).WillByDefault(Return(std::nullopt));
    EXPECT_FALSE(search.resume(root, stack).has_value());
    EXPECT_THAT(stack.frames, IsEmpty());
}

TEST_F(PudResumableWitnessSearchTest, ResumingAfterTheWitnessIsRefutedContinuesWithItsNextSibling) {
    interior(0, {1, 2});
    leaf(1);
    leaf(2);
    ASSERT_THAT(search.resume(root, stack), Optional(pud_test_node(1)));
    ON_CALL(refuted, is_refuted(pud_test_node(1))).WillByDefault(Return(true));
    EXPECT_THAT(search.resume(root, stack), Optional(pud_test_node(2)));
}

TEST_F(PudResumableWitnessSearchTest, ResumingAfterTheWitnessIsUnfoldedDescendsIntoItsChildren) {
    interior(0, {1, 2});
    leaf(1);
    leaf(2);
    ASSERT_THAT(search.resume(root, stack), Optional(pud_test_node(1)));
    ON_CALL(leaves, is_leaf(pud_test_node(1))).WillByDefault(Return(false));
    interior(1, {3});
    leaf(3);
    EXPECT_THAT(search.resume(root, stack), Optional(pud_test_node(3)));
}

TEST_F(PudResumableWitnessSearchTest, ResumingDoesNotRevisitSiblingsBeforeTheWitness) {
    interior(0, {1, 2});
    leaf(1);
    ON_CALL(descend, descend(_, pud_test_node(1))).WillByDefault(Return(std::nullopt));
    leaf(2);
    ASSERT_THAT(search.resume(root, stack), Optional(pud_test_node(2)));
    ON_CALL(descend, descend(_, pud_test_node(1))).WillByDefault(Invoke(&pud_enter_child));
    ON_CALL(refuted, is_refuted(pud_test_node(2))).WillByDefault(Return(true));
    EXPECT_FALSE(search.resume(root, stack).has_value());
}

TEST_F(PudResumableWitnessSearchTest, AdvancingMovesTheRootToTheFirstNodeDescendedInto) {
    interior(0, {1, 2});
    interior(1, {});
    interior(2, {3});
    leaf(3);
    ASSERT_THAT(search.resume(root, stack), Optional(pud_test_node(3)));
    const std::optional<pud_witness_advance> advanced = search.advance(stack);
    ASSERT_TRUE(advanced.has_value());
    EXPECT_EQ(advanced->search_root.node, pud_test_node(2));
    EXPECT_EQ(advanced->next_sibling, 2u);
    ASSERT_EQ(stack.frames.size(), 1u);
    EXPECT_EQ(stack.frames.front().current->node, pud_test_node(3));
}

TEST_F(PudResumableWitnessSearchTest, AnEmptyStackCannotAdvance) {
    EXPECT_FALSE(search.advance(stack).has_value());
}
