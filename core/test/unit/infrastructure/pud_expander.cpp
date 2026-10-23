#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_expander.hpp"
#include "value_objects/pud_node_delta.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockResolve {
    MOCK_METHOD(std::vector<pud_node_delta>, resolve, (pud_node_id, pud_body_goal_id));
};
struct MockMakeChild {
    MOCK_METHOD(pud_node_id, make_child, (const pud_node_delta&));
};
struct MockStoreLiveChildCount {
    MOCK_METHOD(void, store, (pud_node_id, size_t));
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
    NiceMock<MockResolve>, NiceMock<MockMakeChild>, NiceMock<MockStoreLiveChildCount>, NiceMock<MockActivateChild>,
    NiceMock<MockRefuteLeaf>, NiceMock<MockDeactivateNode>, NiceMock<MockCheckLeafRefuted>>;

struct PudExpanderTest : public ::testing::Test {
    NiceMock<MockResolve> resolver;
    NiceMock<MockMakeChild> node_maker;
    NiceMock<MockStoreLiveChildCount> live_child_counts;
    NiceMock<MockActivateChild> activator;
    NiceMock<MockRefuteLeaf> refuter;
    NiceMock<MockDeactivateNode> deactivator;
    NiceMock<MockCheckLeafRefuted> refuted_leaves;
    test_expander_t expander{resolver, node_maker, live_child_counts, activator, refuter, deactivator, refuted_leaves};
    pud_node_delta first{.parent = 4, .call_site = 1, .substitutions = {}, .body_goals = {}, .var_count = 1};
    pud_node_delta second{.parent = 4, .call_site = 1, .substitutions = {}, .body_goals = {}, .var_count = 2};
    std::vector<pud_node_delta> two_deltas{first, second};

    void SetUp() override {
        ON_CALL(resolver, resolve(4, 1)).WillByDefault(Return(two_deltas));
        ON_CALL(node_maker, make_child(first)).WillByDefault(Return(10));
        ON_CALL(node_maker, make_child(second)).WillByDefault(Return(11));
        ON_CALL(deactivator, deactivate(_)).WillByDefault(Return(std::vector<pud_node_id>{}));
        ON_CALL(refuter, refute(_)).WillByDefault(Return(std::vector<pud_node_id>{}));
    }
};

}

TEST_F(PudExpanderTest, EachInferenceBecomesAChildAndEveryChildStartsAlive) {
    InSequence in_order;
    EXPECT_CALL(node_maker, make_child(first)).WillOnce(Return(10));
    EXPECT_CALL(node_maker, make_child(second)).WillOnce(Return(11));
    EXPECT_CALL(live_child_counts, store(4u, 2u));
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
