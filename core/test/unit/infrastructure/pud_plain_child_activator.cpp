#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_plain_child_activator.hpp"
#include "pud_test_node.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "value_objects/pud_inference.hpp"
#include "value_objects/pud_ubs_state.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetNodeLvc {
    MOCK_METHOD(uint32_t, get, (pud_global_node_id));
};
struct MockGetNodeBindings {
    MOCK_METHOD(const pud_ubs_state&, get, (pud_global_node_id));
};
struct MockStoreNodeLvc {
    MOCK_METHOD(void, store, (pud_global_node_id, uint32_t));
};
struct MockStoreNodeBindings {
    MOCK_METHOD(void, store, (pud_global_node_id, pud_ubs_state));
};
struct MockGetInference {
    MOCK_METHOD(const pud_inference&, get_inference, (pud_global_node_id));
};
struct MockGetNodeGoals {
    MOCK_METHOD(const pud_goal_list&, get, (pud_global_node_id));
};
struct MockGetNodeAxiom {
    MOCK_METHOD(pud_global_node_id, get, (pud_global_node_id), (const));
};
struct MockBuildChildGoalList {
    MOCK_METHOD(
        pud_goal_list,
        build,
        (const pud_goal_list&, pud_body_goal_id, (const std::vector<const expr*>&)),
        (const));
};
struct MockStoreNodeGoals {
    MOCK_METHOD(void, store, (pud_global_node_id, pud_goal_list));
};
struct MockStoreNodeAxiom {
    MOCK_METHOD(void, store, (pud_global_node_id, pud_global_node_id));
};
struct MockIncrementLiveChildCount {
    MOCK_METHOD(void, increment, (pud_global_node_id));
};
struct MockBind {
    MOCK_METHOD(pud_ubs_state, bind, (const pud_ubs_state&, pud_global_node_id));
};

using test_activator_t = pud_plain_child_activator<
    NiceMock<MockGetNodeLvc>, NiceMock<MockGetNodeBindings>, NiceMock<MockStoreNodeLvc>,
    NiceMock<MockStoreNodeBindings>, NiceMock<MockGetInference>, NiceMock<MockGetNodeGoals>,
    NiceMock<MockGetNodeAxiom>, NiceMock<MockBuildChildGoalList>, NiceMock<MockStoreNodeGoals>,
    NiceMock<MockStoreNodeAxiom>, NiceMock<MockIncrementLiveChildCount>, NiceMock<MockBind>>;

struct PudPlainChildActivatorTest : public ::testing::Test {
    NiceMock<MockGetNodeLvc> get_lvc;
    NiceMock<MockGetNodeBindings> get_bindings;
    NiceMock<MockStoreNodeLvc> store_lvc;
    NiceMock<MockStoreNodeBindings> store_bindings;
    NiceMock<MockGetInference> inferences;
    NiceMock<MockGetNodeGoals> get_goals;
    NiceMock<MockGetNodeAxiom> get_axiom;
    NiceMock<MockBuildChildGoalList> goal_lists;
    NiceMock<MockStoreNodeGoals> store_goals;
    NiceMock<MockStoreNodeAxiom> store_axiom;
    NiceMock<MockIncrementLiveChildCount> live_children;
    NiceMock<MockBind> binder;
    test_activator_t activator{
        get_lvc, get_bindings, store_lvc, store_bindings, inferences, get_goals, get_axiom, goal_lists,
        store_goals, store_axiom, live_children, binder};
    expr added_goal{.content = expr::var{.index = 5}};
    pud_inference inference{
        .parent = 1, .call_site = 3, .substitutions = {}, .body_goals = {&added_goal}, .var_count = 2};
    pud_goal_list parent_goals{.goals = {}, .next_id = 4};
    pud_goal_list child_goals{.goals = immer::map<pud_body_goal_id, const expr*>{}.set(4, &added_goal), .next_id = 5};
    pud_ubs_state parent_bindings{};
    pud_ubs_state child_bindings{};

    void SetUp() override {
        ON_CALL(inferences, get_inference(pud_test_node(2))).WillByDefault(ReturnRef(inference));
        ON_CALL(get_goals, get(pud_test_node(1))).WillByDefault(ReturnRef(parent_goals));
        ON_CALL(get_bindings, get(pud_test_node(1))).WillByDefault(ReturnRef(parent_bindings));
        ON_CALL(get_lvc, get(pud_test_node(1))).WillByDefault(Return(6u));
        ON_CALL(get_axiom, get(pud_test_node(1))).WillByDefault(Return(pud_test_node(0)));
        ON_CALL(goal_lists, build(_, _, _)).WillByDefault(Return(child_goals));
        ON_CALL(binder, bind(_, _)).WillByDefault(Return(child_bindings));
    }
};

}

TEST_F(PudPlainChildActivatorTest, TheChildsGoalsCloseTheCallSiteAndAddTheInferencesBody) {
    EXPECT_CALL(goal_lists, build(parent_goals, 3u, inference.body_goals)).WillOnce(Return(child_goals));
    EXPECT_CALL(store_goals, store(pud_test_node(2), child_goals));
    activator.activate_child(pud_test_node(1), pud_test_node(2));
}

TEST_F(PudPlainChildActivatorTest, TheChildKeepsItsParentsAxiomAndCountsAsALiveChild) {
    EXPECT_CALL(store_axiom, store(pud_test_node(2), pud_test_node(0)));
    EXPECT_CALL(live_children, increment(pud_test_node(1)));
    activator.activate_child(pud_test_node(1), pud_test_node(2));
}

TEST_F(PudPlainChildActivatorTest, TheChildsLvcAddsTheInferencesVariablesToItsParents) {
    EXPECT_CALL(store_lvc, store(pud_test_node(2), 8u));
    activator.activate_child(pud_test_node(1), pud_test_node(2));
}

TEST_F(PudPlainChildActivatorTest, TheChildsBindingsAreBoundFromItsParents) {
    EXPECT_CALL(binder, bind(::testing::Ref(parent_bindings), pud_test_node(2))).WillOnce(Return(child_bindings));
    EXPECT_CALL(store_bindings, store(pud_test_node(2), child_bindings));
    activator.activate_child(pud_test_node(1), pud_test_node(2));
}

TEST_F(PudPlainChildActivatorTest, AChildIsNeverRefutedOnArrival) {
    EXPECT_FALSE(activator.activate_child(pud_test_node(1), pud_test_node(2)));
}
