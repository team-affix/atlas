#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_activity_mover.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::InSequence;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockActivateChild {
    MOCK_METHOD(bool, activate_child, (pud_global_node_id, pud_global_node_id));
};
struct MockRefuteLeaf {
    MOCK_METHOD(std::vector<pud_global_node_id>, refute, (pud_global_node_id));
};
struct MockDeactivateNode {
    MOCK_METHOD(std::vector<pud_global_node_id>, deactivate, (pud_global_node_id));
};

using test_activity_mover_t =
    pud_activity_mover<NiceMock<MockActivateChild>, NiceMock<MockRefuteLeaf>, NiceMock<MockDeactivateNode>>;

// Parent node 4 hands its activity to children 10 and 11.
struct PudActivityMoverTest : public ::testing::Test {
    NiceMock<MockActivateChild> activator;
    NiceMock<MockRefuteLeaf> refuter;
    NiceMock<MockDeactivateNode> deactivator;
    test_activity_mover_t mover{activator, refuter, deactivator};
    std::vector<pud_global_node_id> children{pud_test_node(10), pud_test_node(11)};

    void SetUp() override {
        ON_CALL(deactivator, deactivate(_)).WillByDefault(Return(std::vector<pud_global_node_id>{}));
        ON_CALL(refuter, refute(_)).WillByDefault(Return(std::vector<pud_global_node_id>{}));
    }
};

}

TEST_F(PudActivityMoverTest, EveryChildIsActivatedBeforeTheParentIsDeactivated) {
    InSequence in_order;
    EXPECT_CALL(activator, activate_child(pud_test_node(4), pud_test_node(10)));
    EXPECT_CALL(activator, activate_child(pud_test_node(4), pud_test_node(11)));
    EXPECT_CALL(deactivator, deactivate(pud_test_node(4)));
    mover.move(pud_test_node(4), children);
}

TEST_F(PudActivityMoverTest, AChildRefutedOnArrivalIsRefutedOnlyOnceEveryChildIsActivated) {
    ON_CALL(activator, activate_child(pud_test_node(4), pud_test_node(10))).WillByDefault(Return(true));
    EXPECT_CALL(refuter, refute(_)).Times(AnyNumber());
    InSequence in_order;
    EXPECT_CALL(activator, activate_child(pud_test_node(4), pud_test_node(10)));
    EXPECT_CALL(activator, activate_child(pud_test_node(4), pud_test_node(11)));
    EXPECT_CALL(refuter, refute(pud_test_node(10)));
    EXPECT_CALL(deactivator, deactivate(pud_test_node(4)));
    mover.move(pud_test_node(4), children);
}

TEST_F(PudActivityMoverTest, AChildNotRefutedOnArrivalIsNotRefuted) {
    ON_CALL(activator, activate_child(pud_test_node(4), pud_test_node(10))).WillByDefault(Return(true));
    EXPECT_CALL(refuter, refute(pud_test_node(10)));
    EXPECT_CALL(refuter, refute(pud_test_node(11))).Times(0);
    mover.move(pud_test_node(4), children);
}

TEST_F(PudActivityMoverTest, EveryLeafTheParentsDeactivationStrandsIsRefuted) {
    ON_CALL(deactivator, deactivate(pud_test_node(4)))
        .WillByDefault(Return(std::vector<pud_global_node_id>{pud_test_node(20), pud_test_node(21)}));
    EXPECT_CALL(refuter, refute(pud_test_node(20)));
    EXPECT_CALL(refuter, refute(pud_test_node(21)));
    mover.move(pud_test_node(4), children);
}

TEST_F(PudActivityMoverTest, EveryRefutedNodeIsReportedArrivalsFirstThenStranded) {
    ON_CALL(activator, activate_child(pud_test_node(4), pud_test_node(11))).WillByDefault(Return(true));
    ON_CALL(refuter, refute(pud_test_node(11)))
        .WillByDefault(Return(std::vector<pud_global_node_id>{pud_test_node(11), pud_test_node(30)}));
    ON_CALL(deactivator, deactivate(pud_test_node(4)))
        .WillByDefault(Return(std::vector<pud_global_node_id>{pud_test_node(20)}));
    ON_CALL(refuter, refute(pud_test_node(20))).WillByDefault(Return(std::vector<pud_global_node_id>{pud_test_node(20)}));
    EXPECT_EQ(
        mover.move(pud_test_node(4), children),
        (std::vector<pud_global_node_id>{pud_test_node(11), pud_test_node(30), pud_test_node(20)}));
}

TEST_F(PudActivityMoverTest, AQuietMoveRefutesNothing) {
    EXPECT_THAT(mover.move(pud_test_node(4), children), IsEmpty());
}
