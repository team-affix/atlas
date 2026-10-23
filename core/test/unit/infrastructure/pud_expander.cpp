#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_expander.hpp"
#include "value_objects/pud_child_delta.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockResolve {
    MOCK_METHOD(std::vector<pud_child_delta>, resolve, (pud_node_id, pud_body_goal_id));
};
struct MockMakeChild {
    MOCK_METHOD(pud_node_id, make_child, (pud_node_id, pud_body_goal_id, const pud_child_delta&));
};
struct MockUnfold {
    MOCK_METHOD(void, unfold, (pud_node_id, pud_body_goal_id, const std::vector<pud_node_id>&));
};
struct MockActivateChild {
    MOCK_METHOD(bool, activate_child, (pud_node_id, pud_node_id));
};
struct MockRefuteLeaf {
    MOCK_METHOD(std::vector<pud_node_id>, refute, (pud_node_id));
};
struct MockDeactivateNode {
    MOCK_METHOD(std::vector<pud_node_id>, deactivate, (pud_node_id));
};
struct MockCheckLeafRefuted {
    MOCK_METHOD(bool, check_refuted, (pud_node_id));
};

using test_expander_t = pud_expander<
    NiceMock<MockResolve>, NiceMock<MockMakeChild>, NiceMock<MockUnfold>, NiceMock<MockActivateChild>,
    NiceMock<MockRefuteLeaf>, NiceMock<MockDeactivateNode>, NiceMock<MockCheckLeafRefuted>>;

struct PudExpanderTest : public ::testing::Test {
    NiceMock<MockResolve> resolver;
    NiceMock<MockMakeChild> node_maker;
    NiceMock<MockUnfold> unfolder;
    NiceMock<MockActivateChild> activator;
    NiceMock<MockRefuteLeaf> refuter;
    NiceMock<MockDeactivateNode> deactivator;
    NiceMock<MockCheckLeafRefuted> refuted_leaves;
    test_expander_t expander{resolver, node_maker, unfolder, activator, refuter, deactivator, refuted_leaves};
    pud_child_delta first{.substitutions = {}, .body_goals = {}, .var_count = 1};
    pud_child_delta second{.substitutions = {}, .body_goals = {}, .var_count = 2};
    std::vector<pud_child_delta> two_deltas{first, second};

    void SetUp() override {
        ON_CALL(resolver, resolve(4, 1)).WillByDefault(Return(two_deltas));
        ON_CALL(node_maker, make_child(4, 1, first)).WillByDefault(Return(10));
        ON_CALL(node_maker, make_child(4, 1, second)).WillByDefault(Return(11));
        ON_CALL(deactivator, deactivate(_)).WillByDefault(Return(std::vector<pud_node_id>{}));
        ON_CALL(refuter, refute(_)).WillByDefault(Return(std::vector<pud_node_id>{}));
    }
};

}

TEST_F(PudExpanderTest, EachCandidateBecomesAChildAndTheLeafUnfoldsIntoThem) {
    InSequence in_order;
    EXPECT_CALL(node_maker, make_child(4, 1, first)).WillOnce(Return(10));
    EXPECT_CALL(node_maker, make_child(4, 1, second)).WillOnce(Return(11));
    EXPECT_CALL(unfolder, unfold(4, 1, std::vector<pud_node_id>{10, 11}));
    expander.expand(4, 1);
}

TEST_F(PudExpanderTest, EveryChildIsActivatedBeforeTheLeafIsDeactivated) {
    InSequence in_order;
    EXPECT_CALL(activator, activate_child(4, 10));
    EXPECT_CALL(activator, activate_child(4, 11));
    EXPECT_CALL(deactivator, deactivate(4u));
    expander.expand(4, 1);
}

TEST_F(PudExpanderTest, AChildRefutedOnArrivalIsRefutedBeforeTheNextChildIsActivated) {
    ON_CALL(activator, activate_child(4, 10)).WillByDefault(Return(true));
    InSequence in_order;
    EXPECT_CALL(activator, activate_child(4, 10));
    EXPECT_CALL(refuter, refute(10u));
    EXPECT_CALL(activator, activate_child(4, 11));
    EXPECT_CALL(deactivator, deactivate(4u));
    expander.expand(4, 1);
}

TEST_F(PudExpanderTest, EveryLeafTheLeafsDeactivationStrandsIsRefuted) {
    ON_CALL(deactivator, deactivate(4u)).WillByDefault(Return(std::vector<pud_node_id>{20, 21}));
    EXPECT_CALL(refuter, refute(20u));
    EXPECT_CALL(refuter, refute(21u));
    expander.expand(4, 1);
}

TEST_F(PudExpanderTest, TheChangeHoldsTheLiveChildrenAndEveryRefutedLeaf) {
    ON_CALL(activator, activate_child(4, 11)).WillByDefault(Return(true));
    ON_CALL(refuter, refute(11u)).WillByDefault(Return(std::vector<pud_node_id>{11, 20}));
    ON_CALL(refuted_leaves, check_refuted(11u)).WillByDefault(Return(true));
    EXPECT_EQ(expander.expand(4, 1), (pud_frontier_change{.offspring = {10}, .refuted = {11, 20}}));
}
