#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_expander.hpp"
#include "value_objects/pud_node_delta.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockResolve {
    MOCK_METHOD(std::vector<pud_node_delta>, resolve, (pud_global_node_id, pud_body_goal_id));
};
struct MockStoreInference {
    MOCK_METHOD(pud_global_node_id, store_inference, (pud_module_id, pud_node_delta));
};
struct MockIndexInference {
    MOCK_METHOD(void, index_inference, (pud_global_node_id));
};
struct MockStoreLiveChildCount {
    MOCK_METHOD(void, store, (pud_global_node_id, size_t));
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

using test_expander_t = pud_expander<
    NiceMock<MockResolve>, NiceMock<MockStoreInference>, NiceMock<MockIndexInference>,
    NiceMock<MockStoreLiveChildCount>, NiceMock<MockActivateChild>, NiceMock<MockRefuteLeaf>,
    NiceMock<MockDeactivateNode>, NiceMock<MockCheckNodeRefuted>>;

struct PudExpanderTest : public ::testing::Test {
    NiceMock<MockResolve> resolver;
    NiceMock<MockStoreInference> store_inference;
    NiceMock<MockIndexInference> index_inference;
    NiceMock<MockStoreLiveChildCount> live_child_counts;
    NiceMock<MockActivateChild> activator;
    NiceMock<MockRefuteLeaf> refuter;
    NiceMock<MockDeactivateNode> deactivator;
    NiceMock<MockCheckNodeRefuted> refuted_nodes;
    test_expander_t expander{
        resolver, store_inference, index_inference, live_child_counts, activator, refuter, deactivator, refuted_nodes};
    pud_node_delta first{.parent = 4, .call_site = 1, .substitutions = {}, .body_goals = {}, .var_count = 1};
    pud_node_delta second{.parent = 4, .call_site = 1, .substitutions = {}, .body_goals = {}, .var_count = 2};
    std::vector<pud_node_delta> two_deltas{first, second};

    void SetUp() override {
        ON_CALL(resolver, resolve(pud_test_node(4), 1)).WillByDefault(Return(two_deltas));
        ON_CALL(store_inference, store_inference(k_test_module, first)).WillByDefault(Return(pud_test_node(10)));
        ON_CALL(store_inference, store_inference(k_test_module, second)).WillByDefault(Return(pud_test_node(11)));
        ON_CALL(deactivator, deactivate(_)).WillByDefault(Return(std::vector<pud_global_node_id>{}));
        ON_CALL(refuter, refute(_)).WillByDefault(Return(std::vector<pud_global_node_id>{}));
    }
};

}

TEST_F(PudExpanderTest, EachInferenceBecomesAChildAndEveryChildStartsAlive) {
    InSequence in_order;
    EXPECT_CALL(store_inference, store_inference(k_test_module, first)).WillOnce(Return(pud_test_node(10)));
    EXPECT_CALL(store_inference, store_inference(k_test_module, second)).WillOnce(Return(pud_test_node(11)));
    EXPECT_CALL(live_child_counts, store(pud_test_node(4), 2u));
    expander.expand(pud_test_node(4), 1);
}

TEST_F(PudExpanderTest, EveryInferenceIsStoredInTheLeafsModuleAndIndexedBeforeTheNextIsStored) {
    const pud_module_id other_module = k_test_module + 1;
    const pud_global_node_id leaf{.module = other_module, .node = 4};
    const pud_global_node_id first_child{.module = other_module, .node = 10};
    const pud_global_node_id second_child{.module = other_module, .node = 11};
    ON_CALL(resolver, resolve(leaf, 1)).WillByDefault(Return(two_deltas));
    InSequence in_order;
    EXPECT_CALL(store_inference, store_inference(other_module, first)).WillOnce(Return(first_child));
    EXPECT_CALL(index_inference, index_inference(first_child));
    EXPECT_CALL(store_inference, store_inference(other_module, second)).WillOnce(Return(second_child));
    EXPECT_CALL(index_inference, index_inference(second_child));
    expander.expand(leaf, 1);
}

TEST_F(PudExpanderTest, EveryChildIsActivatedBeforeTheLeafIsDeactivated) {
    InSequence in_order;
    EXPECT_CALL(activator, activate_child(pud_test_node(4), pud_test_node(10)));
    EXPECT_CALL(activator, activate_child(pud_test_node(4), pud_test_node(11)));
    EXPECT_CALL(deactivator, deactivate(pud_test_node(4)));
    expander.expand(pud_test_node(4), 1);
}

TEST_F(PudExpanderTest, AChildRefutedOnArrivalIsRefutedBeforeTheNextChildIsActivated) {
    ON_CALL(activator, activate_child(pud_test_node(4), pud_test_node(10))).WillByDefault(Return(true));
    InSequence in_order;
    EXPECT_CALL(activator, activate_child(pud_test_node(4), pud_test_node(10)));
    EXPECT_CALL(refuter, refute(pud_test_node(10)));
    EXPECT_CALL(activator, activate_child(pud_test_node(4), pud_test_node(11)));
    EXPECT_CALL(deactivator, deactivate(pud_test_node(4)));
    expander.expand(pud_test_node(4), 1);
}

TEST_F(PudExpanderTest, EveryLeafTheLeafsDeactivationStrandsIsRefuted) {
    ON_CALL(deactivator, deactivate(pud_test_node(4)))
        .WillByDefault(Return(std::vector<pud_global_node_id>{pud_test_node(20), pud_test_node(21)}));
    EXPECT_CALL(refuter, refute(pud_test_node(20)));
    EXPECT_CALL(refuter, refute(pud_test_node(21)));
    expander.expand(pud_test_node(4), 1);
}

TEST_F(PudExpanderTest, TheChangeHoldsTheLiveChildrenAndEveryRefutedLeaf) {
    ON_CALL(activator, activate_child(pud_test_node(4), pud_test_node(11))).WillByDefault(Return(true));
    ON_CALL(refuter, refute(pud_test_node(11)))
        .WillByDefault(Return(std::vector<pud_global_node_id>{pud_test_node(11), pud_test_node(20)}));
    ON_CALL(refuted_nodes, check_refuted(pud_test_node(11))).WillByDefault(Return(true));
    EXPECT_EQ(expander.expand(pud_test_node(4), 1), (pud_frontier_change{
        .offspring = {pud_test_node(10)}, .refuted = {pud_test_node(11), pud_test_node(20)}}));
}
