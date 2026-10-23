#include <optional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_refutation_propagator.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockGetNodeParent {
    MOCK_METHOD(std::optional<pud_global_node_id>, get_parent, (pud_global_node_id));
};
struct MockDecrementLiveChildCount {
    MOCK_METHOD(size_t, decrement, (pud_global_node_id));
};
struct MockMarkNodeRefuted {
    MOCK_METHOD(void, mark_refuted, (pud_global_node_id));
};

using test_propagator_t = pud_refutation_propagator<
    NiceMock<MockGetNodeParent>, NiceMock<MockDecrementLiveChildCount>, NiceMock<MockMarkNodeRefuted>>;

// A chain 1 <- 2 <- 3: node 1 is the root.
struct PudRefutationPropagatorTest : public ::testing::Test {
    NiceMock<MockGetNodeParent> parents;
    NiceMock<MockDecrementLiveChildCount> counts;
    NiceMock<MockMarkNodeRefuted> marks;
    test_propagator_t propagator{parents, counts, marks};
    void SetUp() override {
        ON_CALL(parents, get_parent(pud_test_node(1))).WillByDefault(Return(std::nullopt));
        ON_CALL(parents, get_parent(pud_test_node(2))).WillByDefault(Return(pud_test_node(1)));
        ON_CALL(parents, get_parent(pud_test_node(3))).WillByDefault(Return(pud_test_node(2)));
    }
};

}

TEST_F(PudRefutationPropagatorTest, ADeadRootAffectsNoOtherNode) {
    EXPECT_CALL(counts, decrement(_)).Times(0);
    EXPECT_CALL(marks, mark_refuted(_)).Times(0);
    propagator.refute(pud_test_node(1));
}

TEST_F(PudRefutationPropagatorTest, AParentWithAnotherLiveChildStopsThePropagation) {
    EXPECT_CALL(counts, decrement(pud_test_node(2))).WillOnce(Return(1));
    EXPECT_CALL(counts, decrement(pud_test_node(1))).Times(0);
    propagator.refute(pud_test_node(3));
}

TEST_F(PudRefutationPropagatorTest, AParentLosingItsLastLiveChildDiesInTurn) {
    InSequence in_order;
    EXPECT_CALL(counts, decrement(pud_test_node(2))).WillOnce(Return(0));
    EXPECT_CALL(counts, decrement(pud_test_node(1))).WillOnce(Return(0));
    propagator.refute(pud_test_node(3));
}

TEST_F(PudRefutationPropagatorTest, AnAncestorWhoseLastLiveChildDiesIsMarkedRefuted) {
    ON_CALL(counts, decrement(pud_test_node(2))).WillByDefault(Return(0));
    ON_CALL(counts, decrement(pud_test_node(1))).WillByDefault(Return(1));
    EXPECT_CALL(marks, mark_refuted(pud_test_node(2)));
    propagator.refute(pud_test_node(3));
}

TEST_F(PudRefutationPropagatorTest, AnAncestorKeepingALiveChildIsNotMarkedAndTheWalkStopsThere) {
    EXPECT_CALL(counts, decrement(pud_test_node(2))).WillOnce(Return(1));
    EXPECT_CALL(marks, mark_refuted(_)).Times(0);
    EXPECT_CALL(parents, get_parent(pud_test_node(3)));
    EXPECT_CALL(parents, get_parent(pud_test_node(2))).Times(0);
    EXPECT_CALL(counts, decrement(pud_test_node(1))).Times(0);
    propagator.refute(pud_test_node(3));
}

TEST_F(PudRefutationPropagatorTest, AChainMarksEveryDeadAncestorInOrderBottomUp) {
    InSequence in_order;
    EXPECT_CALL(counts, decrement(pud_test_node(2))).WillOnce(Return(0));
    EXPECT_CALL(marks, mark_refuted(pud_test_node(2)));
    EXPECT_CALL(counts, decrement(pud_test_node(1))).WillOnce(Return(0));
    EXPECT_CALL(marks, mark_refuted(pud_test_node(1)));
    propagator.refute(pud_test_node(3));
}
