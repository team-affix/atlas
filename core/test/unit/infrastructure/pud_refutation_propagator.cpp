#include <optional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_refutation_propagator.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetNodeParent {
    MOCK_METHOD(const std::optional<pud_node_id>&, get, (pud_node_id));
};
struct MockDecrementLiveChildCount {
    MOCK_METHOD(size_t, decrement, (pud_node_id));
};

struct PudRefutationPropagatorTest : public ::testing::Test {
    NiceMock<MockGetNodeParent> parents;
    NiceMock<MockDecrementLiveChildCount> counts;
    pud_refutation_propagator<NiceMock<MockGetNodeParent>, NiceMock<MockDecrementLiveChildCount>> propagator{parents, counts};
    const std::optional<pud_node_id> no_parent;
    const std::optional<pud_node_id> parent_one{1};
    const std::optional<pud_node_id> parent_two{2};

    void SetUp() override {
        ON_CALL(parents, get(1)).WillByDefault(ReturnRef(no_parent));
        ON_CALL(parents, get(2)).WillByDefault(ReturnRef(parent_one));
        ON_CALL(parents, get(3)).WillByDefault(ReturnRef(parent_two));
    }
};

}

TEST_F(PudRefutationPropagatorTest, ADeadRootAffectsNoOtherNode) {
    EXPECT_CALL(counts, decrement(_)).Times(0);
    propagator.refute(1);
}

TEST_F(PudRefutationPropagatorTest, AParentWithAnotherLiveChildStopsThePropagation) {
    EXPECT_CALL(counts, decrement(2)).WillOnce(Return(1));
    EXPECT_CALL(counts, decrement(1)).Times(0);
    propagator.refute(3);
}

TEST_F(PudRefutationPropagatorTest, AParentLosingItsLastLiveChildDiesInTurn) {
    InSequence in_order;
    EXPECT_CALL(counts, decrement(2)).WillOnce(Return(0));
    EXPECT_CALL(counts, decrement(1)).WillOnce(Return(0));
    propagator.refute(3);
}
