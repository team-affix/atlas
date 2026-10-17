#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_node_refuter.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::InSequence;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockFindLiveLeaves {
    MOCK_METHOD(std::vector<pud_node_id>, find_live_leaves, (pud_node_id));
};

struct MockMarkLeafRefuted {
    MOCK_METHOD(void, mark_refuted, (pud_node_id));
};

struct MockCheckLeafOpen {
    MOCK_METHOD(bool, check_leaf_open, (pud_node_id));
};

struct MockMarkLeafClosed {
    MOCK_METHOD(void, mark_leaf_closed, (pud_node_id));
};

struct MockPropagateRefutation {
    MOCK_METHOD(void, refute, (pud_node_id));
};

struct MockEraseNodeDescent {
    MOCK_METHOD(void, erase, (pud_node_id));
};

struct MockInvalidateLeaves {
    MOCK_METHOD(std::vector<pud_forced_query_propagation>, invalidate, (const std::vector<pud_node_id>&));
};

struct MockGetQueryHeadNode {
    MOCK_METHOD(pud_node_id, get, (pud_mhq_head_id));
};

using test_refuter_t = pud_node_refuter<
    NiceMock<MockFindLiveLeaves>,
    NiceMock<MockMarkLeafRefuted>,
    NiceMock<MockCheckLeafOpen>,
    NiceMock<MockMarkLeafClosed>,
    NiceMock<MockPropagateRefutation>,
    NiceMock<MockEraseNodeDescent>,
    NiceMock<MockInvalidateLeaves>,
    NiceMock<MockGetQueryHeadNode>>;

struct PudNodeRefuterTest : public ::testing::Test {
    NiceMock<MockFindLiveLeaves> live_leaves;
    NiceMock<MockMarkLeafRefuted> refuted_leaves;
    NiceMock<MockCheckLeafOpen> check_open;
    NiceMock<MockMarkLeafClosed> close_leaf;
    NiceMock<MockPropagateRefutation> propagator;
    NiceMock<MockEraseNodeDescent> descents;
    NiceMock<MockInvalidateLeaves> invalidator;
    NiceMock<MockGetQueryHeadNode> head_nodes;
    test_refuter_t refuter{
        live_leaves, refuted_leaves, check_open, close_leaf, propagator, descents, invalidator, head_nodes};
    pud_node_id node = 1;
    pud_node_id leaf_a = 2;
    pud_node_id leaf_b = 3;
    pud_node_id opener = 4;
    pud_node_id opener_leaf = 5;

    void SetUp() override {
        ON_CALL(live_leaves, find_live_leaves(node)).WillByDefault(Return(std::vector<pud_node_id>{leaf_a, leaf_b}));
        ON_CALL(live_leaves, find_live_leaves(opener)).WillByDefault(Return(std::vector<pud_node_id>{opener_leaf}));
    }
};

TEST_F(PudNodeRefuterTest, EveryLiveLeafIsMarkedPropagatedAndReleased) {
    EXPECT_CALL(refuted_leaves, mark_refuted(leaf_a));
    EXPECT_CALL(refuted_leaves, mark_refuted(leaf_b));
    EXPECT_CALL(propagator, refute(leaf_a));
    EXPECT_CALL(propagator, refute(leaf_b));
    EXPECT_CALL(descents, erase(leaf_a));
    EXPECT_CALL(descents, erase(leaf_b));
    refuter.refute(node);
}

TEST_F(PudNodeRefuterTest, LeafIsMarkedBeforeItsRefutationPropagates) {
    EXPECT_CALL(refuted_leaves, mark_refuted(_)).Times(::testing::AnyNumber());
    EXPECT_CALL(propagator, refute(_)).Times(::testing::AnyNumber());
    InSequence in_order;
    EXPECT_CALL(refuted_leaves, mark_refuted(leaf_a));
    EXPECT_CALL(propagator, refute(leaf_a));
    refuter.refute(node);
}

TEST_F(PudNodeRefuterTest, LeavesAreInvalidatedTogetherAfterEveryOneIsMarked) {
    InSequence in_order;
    EXPECT_CALL(refuted_leaves, mark_refuted(leaf_a));
    EXPECT_CALL(refuted_leaves, mark_refuted(leaf_b));
    EXPECT_CALL(invalidator, invalidate(ElementsAre(leaf_a, leaf_b)));
    refuter.refute(node);
}

TEST_F(PudNodeRefuterTest, RefutedQueryRefutesTheNodeThatOpenedIt) {
    EXPECT_CALL(invalidator, invalidate(ElementsAre(leaf_a, leaf_b)))
        .WillOnce(Return(std::vector<pud_forced_query_propagation>{pud_refuted_query{.head_id = 9}}));
    EXPECT_CALL(head_nodes, get(9u)).WillOnce(Return(opener));
    EXPECT_CALL(refuted_leaves, mark_refuted(_)).Times(::testing::AnyNumber());
    EXPECT_CALL(refuted_leaves, mark_refuted(opener_leaf));
    EXPECT_CALL(invalidator, invalidate(ElementsAre(opener_leaf)));
    refuter.refute(node);
}

TEST_F(PudNodeRefuterTest, UnitQueryRefutesNothingMore) {
    EXPECT_CALL(invalidator, invalidate(ElementsAre(leaf_a, leaf_b)))
        .WillOnce(Return(std::vector<pud_forced_query_propagation>{pud_unit_query{.head_id = 9}}));
    EXPECT_CALL(head_nodes, get(_)).Times(0);
    refuter.refute(node);
}

TEST_F(PudNodeRefuterTest, CascadeContinuesUntilNothingMoreIsForced) {
    pud_node_id second_opener = 6;
    ON_CALL(live_leaves, find_live_leaves(second_opener)).WillByDefault(Return(std::vector<pud_node_id>{}));
    EXPECT_CALL(invalidator, invalidate(ElementsAre(leaf_a, leaf_b)))
        .WillOnce(Return(std::vector<pud_forced_query_propagation>{pud_refuted_query{.head_id = 9}}));
    EXPECT_CALL(head_nodes, get(9u)).WillOnce(Return(opener));
    EXPECT_CALL(invalidator, invalidate(ElementsAre(opener_leaf)))
        .WillOnce(Return(std::vector<pud_forced_query_propagation>{pud_refuted_query{.head_id = 10}}));
    EXPECT_CALL(head_nodes, get(10u)).WillOnce(Return(second_opener));
    EXPECT_CALL(invalidator, invalidate(IsEmpty())).WillOnce(Return(std::vector<pud_forced_query_propagation>{}));
    refuter.refute(node);
}

TEST_F(PudNodeRefuterTest, AlreadyRefutedNodeMarksNothing) {
    EXPECT_CALL(live_leaves, find_live_leaves(node)).WillOnce(Return(std::vector<pud_node_id>{}));
    EXPECT_CALL(refuted_leaves, mark_refuted(_)).Times(0);
    EXPECT_CALL(propagator, refute(_)).Times(0);
    refuter.refute(node);
}

}  // namespace

// ---------- open leaves ----------

TEST_F(PudNodeRefuterTest, OpenLiveLeavesAreClosed) {
    ON_CALL(check_open, check_leaf_open(_)).WillByDefault(Return(true));
    EXPECT_CALL(close_leaf, mark_leaf_closed(leaf_a));
    EXPECT_CALL(close_leaf, mark_leaf_closed(leaf_b));
    refuter.refute(node);
}

TEST_F(PudNodeRefuterTest, LeavesThatWereNotOpenAreNotClosedAgain) {
    ON_CALL(check_open, check_leaf_open(leaf_a)).WillByDefault(Return(true));
    ON_CALL(check_open, check_leaf_open(leaf_b)).WillByDefault(Return(false));
    EXPECT_CALL(close_leaf, mark_leaf_closed(leaf_a));
    EXPECT_CALL(close_leaf, mark_leaf_closed(leaf_b)).Times(0);
    refuter.refute(node);
}

TEST_F(PudNodeRefuterTest, LeafIsClosedOnlyAfterItIsMarkedRefuted) {
    ON_CALL(check_open, check_leaf_open(_)).WillByDefault(Return(true));
    InSequence in_order;
    EXPECT_CALL(refuted_leaves, mark_refuted(leaf_a));
    EXPECT_CALL(close_leaf, mark_leaf_closed(leaf_a));
    EXPECT_CALL(refuted_leaves, mark_refuted(leaf_b));
    EXPECT_CALL(close_leaf, mark_leaf_closed(leaf_b));
    refuter.refute(node);
}

TEST_F(PudNodeRefuterTest, LeavesOfARefutedOpenerAreClosedToo) {
    ON_CALL(check_open, check_leaf_open(_)).WillByDefault(Return(true));
    ON_CALL(invalidator, invalidate(std::vector<pud_node_id>{leaf_a, leaf_b}))
        .WillByDefault(Return(std::vector<pud_forced_query_propagation>{pud_refuted_query{.head_id = 9}}));
    ON_CALL(head_nodes, get(9)).WillByDefault(Return(opener));
    EXPECT_CALL(close_leaf, mark_leaf_closed(leaf_a));
    EXPECT_CALL(close_leaf, mark_leaf_closed(leaf_b));
    EXPECT_CALL(close_leaf, mark_leaf_closed(opener_leaf));
    refuter.refute(node);
}
