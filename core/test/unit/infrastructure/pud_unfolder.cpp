#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_unfolder.hpp"
#include "value_objects/pud_inference.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockResolve {
    MOCK_METHOD(std::vector<pud_inference>, resolve, (pud_global_node_id, pud_body_goal_id));
};
struct MockStoreInference {
    MOCK_METHOD(pud_global_node_id, store_inference, (pud_module_id, pud_inference));
};
struct MockActivateChild {
    MOCK_METHOD(bool, activate_child, (pud_global_node_id, pud_global_node_id));
};
struct MockRefuteLeaf {
    MOCK_METHOD(std::vector<pud_global_node_id>, refute, (pud_global_node_id));
};
struct MockDeactivateNode {
    MOCK_METHOD(std::vector<pud_global_node_id>, deactivate, (pud_global_node_id));
};
struct MockCheckNodeRefuted {
    MOCK_METHOD(bool, check_refuted, (pud_global_node_id));
};

using test_unfolder_t = pud_unfolder<
    NiceMock<MockResolve>, NiceMock<MockStoreInference>, NiceMock<MockActivateChild>, NiceMock<MockRefuteLeaf>,
    NiceMock<MockDeactivateNode>, NiceMock<MockCheckNodeRefuted>>;

struct PudUnfolderTest : public ::testing::Test {
    NiceMock<MockResolve> resolver;
    NiceMock<MockStoreInference> store_inference;
    NiceMock<MockActivateChild> activator;
    NiceMock<MockRefuteLeaf> refuter;
    NiceMock<MockDeactivateNode> deactivator;
    NiceMock<MockCheckNodeRefuted> refuted_nodes;
    test_unfolder_t unfolder{resolver, store_inference, activator, refuter, deactivator, refuted_nodes};
    pud_inference first{.parent = 4, .call_site = 1, .substitutions = {}, .body_goals = {}, .var_count = 1};
    pud_inference second{.parent = 4, .call_site = 1, .substitutions = {}, .body_goals = {}, .var_count = 2};
    std::vector<pud_inference> two_deltas{first, second};

    void SetUp() override {
        ON_CALL(resolver, resolve(pud_test_node(4), 1)).WillByDefault(Return(two_deltas));
        ON_CALL(store_inference, store_inference(k_test_module, first)).WillByDefault(Return(pud_test_node(10)));
        ON_CALL(store_inference, store_inference(k_test_module, second)).WillByDefault(Return(pud_test_node(11)));
        ON_CALL(deactivator, deactivate(_)).WillByDefault(Return(std::vector<pud_global_node_id>{}));
        ON_CALL(refuter, refute(_)).WillByDefault(Return(std::vector<pud_global_node_id>{}));
    }
};

}

TEST_F(PudUnfolderTest, EveryInferenceIsStoredInTheLeafsModuleInTheOrderResolved) {
    const pud_module_id other_module = k_test_module + 1;
    const pud_global_node_id leaf{.module = other_module, .node = 4};
    ON_CALL(resolver, resolve(leaf, 1)).WillByDefault(Return(two_deltas));
    InSequence in_order;
    EXPECT_CALL(store_inference, store_inference(other_module, first))
        .WillOnce(Return(pud_global_node_id{.module = other_module, .node = 10}));
    EXPECT_CALL(store_inference, store_inference(other_module, second))
        .WillOnce(Return(pud_global_node_id{.module = other_module, .node = 11}));
    unfolder.unfold(leaf, 1);
}

TEST_F(PudUnfolderTest, EveryChildIsActivatedBeforeTheLeafIsDeactivated) {
    InSequence in_order;
    EXPECT_CALL(activator, activate_child(pud_test_node(4), pud_test_node(10)));
    EXPECT_CALL(activator, activate_child(pud_test_node(4), pud_test_node(11)));
    EXPECT_CALL(deactivator, deactivate(pud_test_node(4)));
    unfolder.unfold(pud_test_node(4), 1);
}

TEST_F(PudUnfolderTest, AChildRefutedOnArrivalIsRefutedOnlyOnceEveryChildIsActivated) {
    ON_CALL(activator, activate_child(pud_test_node(4), pud_test_node(10))).WillByDefault(Return(true));
    EXPECT_CALL(refuter, refute(_)).Times(AnyNumber());
    InSequence in_order;
    EXPECT_CALL(activator, activate_child(pud_test_node(4), pud_test_node(10)));
    EXPECT_CALL(activator, activate_child(pud_test_node(4), pud_test_node(11)));
    EXPECT_CALL(refuter, refute(pud_test_node(10)));
    EXPECT_CALL(deactivator, deactivate(pud_test_node(4)));
    unfolder.unfold(pud_test_node(4), 1);
}

TEST_F(PudUnfolderTest, AChildNotRefutedOnArrivalIsNotRefuted) {
    ON_CALL(activator, activate_child(pud_test_node(4), pud_test_node(10))).WillByDefault(Return(true));
    EXPECT_CALL(refuter, refute(pud_test_node(10)));
    EXPECT_CALL(refuter, refute(pud_test_node(11))).Times(0);
    unfolder.unfold(pud_test_node(4), 1);
}

TEST_F(PudUnfolderTest, EveryLeafTheLeafsDeactivationStrandsIsRefuted) {
    ON_CALL(deactivator, deactivate(pud_test_node(4)))
        .WillByDefault(Return(std::vector<pud_global_node_id>{pud_test_node(20), pud_test_node(21)}));
    EXPECT_CALL(refuter, refute(pud_test_node(20)));
    EXPECT_CALL(refuter, refute(pud_test_node(21)));
    unfolder.unfold(pud_test_node(4), 1);
}

TEST_F(PudUnfolderTest, TheChangeHoldsTheLiveChildrenAndEveryRefutedLeaf) {
    ON_CALL(activator, activate_child(pud_test_node(4), pud_test_node(11))).WillByDefault(Return(true));
    ON_CALL(refuter, refute(pud_test_node(11)))
        .WillByDefault(Return(std::vector<pud_global_node_id>{pud_test_node(11), pud_test_node(20)}));
    ON_CALL(refuted_nodes, check_refuted(pud_test_node(11))).WillByDefault(Return(true));
    EXPECT_EQ(unfolder.unfold(pud_test_node(4), 1), (pud_frontier_change{
        .offspring = {pud_test_node(10)}, .refuted = {pud_test_node(11), pud_test_node(20)}}));
}
