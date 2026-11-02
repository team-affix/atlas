#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_unfolder.hpp"
#include "value_objects/pud_inference.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
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
struct MockMoveActivity {
    MOCK_METHOD(std::vector<pud_global_node_id>, move, (pud_global_node_id, (const std::vector<pud_global_node_id>&)));
};
struct MockIsNodeRefuted {
    MOCK_METHOD(bool, is_refuted, (pud_global_node_id));
};

using test_unfolder_t = pud_unfolder<
    NiceMock<MockResolve>, NiceMock<MockStoreInference>, NiceMock<MockMoveActivity>, NiceMock<MockIsNodeRefuted>>;

struct PudUnfolderTest : public ::testing::Test {
    NiceMock<MockResolve> resolver;
    NiceMock<MockStoreInference> store_inference;
    NiceMock<MockMoveActivity> mover;
    NiceMock<MockIsNodeRefuted> refuted_nodes;
    test_unfolder_t unfolder{resolver, store_inference, mover, refuted_nodes};
    pud_inference first{.parent = 4, .call_site = 1, .substitutions = {}, .body_goals = {}, .var_count = 1};
    pud_inference second{.parent = 4, .call_site = 1, .substitutions = {}, .body_goals = {}, .var_count = 2};
    std::vector<pud_inference> two_deltas{first, second};

    void SetUp() override {
        ON_CALL(resolver, resolve(pud_test_node(4), 1)).WillByDefault(Return(two_deltas));
        ON_CALL(store_inference, store_inference(k_test_module, first)).WillByDefault(Return(pud_test_node(10)));
        ON_CALL(store_inference, store_inference(k_test_module, second)).WillByDefault(Return(pud_test_node(11)));
        ON_CALL(mover, move(_, _)).WillByDefault(Return(std::vector<pud_global_node_id>{}));
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

TEST_F(PudUnfolderTest, ActivityMovesFromTheLeafToEveryStoredChildOnceTheyAreAllStored) {
    InSequence in_order;
    EXPECT_CALL(store_inference, store_inference(k_test_module, first)).WillOnce(Return(pud_test_node(10)));
    EXPECT_CALL(store_inference, store_inference(k_test_module, second)).WillOnce(Return(pud_test_node(11)));
    EXPECT_CALL(mover, move(pud_test_node(4), std::vector<pud_global_node_id>{pud_test_node(10), pud_test_node(11)}));
    unfolder.unfold(pud_test_node(4), 1);
}

TEST_F(PudUnfolderTest, TheChangeHoldsTheLiveChildrenAndEveryRefutedLeafTheMoveReports) {
    ON_CALL(mover, move(pud_test_node(4), _))
        .WillByDefault(Return(std::vector<pud_global_node_id>{pud_test_node(11), pud_test_node(20)}));
    ON_CALL(refuted_nodes, is_refuted(pud_test_node(11))).WillByDefault(Return(true));
    EXPECT_EQ(unfolder.unfold(pud_test_node(4), 1), (pud_frontier_change{
        .offspring = {pud_test_node(10)}, .refuted = {pud_test_node(11), pud_test_node(20)}}));
}
