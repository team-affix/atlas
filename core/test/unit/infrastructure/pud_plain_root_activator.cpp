#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_plain_root_activator.hpp"
#include "pud_test_node.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "value_objects/pud_ubs_state.hpp"
#include "value_objects/rule.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetAxiom {
    MOCK_METHOD(const rule&, get_axiom, (pud_global_node_id), (const));
};
struct MockBuildRootGoalList {
    MOCK_METHOD(pud_goal_list, build, ((const std::vector<const expr*>&)), (const));
};
struct MockStoreNodeGoals {
    MOCK_METHOD(void, store, (pud_global_node_id, pud_goal_list));
};
struct MockStoreNodeAxiom {
    MOCK_METHOD(void, store, (pud_global_node_id, pud_global_node_id));
};
struct MockStoreNodeLvc {
    MOCK_METHOD(void, store, (pud_global_node_id, uint32_t));
};
struct MockStoreNodeBindings {
    MOCK_METHOD(void, store, (pud_global_node_id, pud_ubs_state));
};

using test_activator_t = pud_plain_root_activator<
    NiceMock<MockGetAxiom>, NiceMock<MockBuildRootGoalList>, NiceMock<MockStoreNodeGoals>,
    NiceMock<MockStoreNodeAxiom>, NiceMock<MockStoreNodeLvc>, NiceMock<MockStoreNodeBindings>>;

struct PudPlainRootActivatorTest : public ::testing::Test {
    NiceMock<MockGetAxiom> axioms;
    NiceMock<MockBuildRootGoalList> goal_lists;
    NiceMock<MockStoreNodeGoals> goals;
    NiceMock<MockStoreNodeAxiom> node_axioms;
    NiceMock<MockStoreNodeLvc> lvcs;
    NiceMock<MockStoreNodeBindings> bindings;
    test_activator_t activator{axioms, goal_lists, goals, node_axioms, lvcs, bindings};
    expr body_goal{.content = expr::var{.index = 0}};
    expr head{.content = expr::var{.index = 1}};
    rule axiom{&head, {&body_goal}, 4};
    pud_goal_list built{.goals = immer::map<pud_body_goal_id, const expr*>{}.set(0, &body_goal), .next_id = 1};

    void SetUp() override {
        ON_CALL(axioms, get_axiom(pud_test_node(2))).WillByDefault(ReturnRef(axiom));
        ON_CALL(goal_lists, build(_)).WillByDefault(Return(built));
    }
};

}

TEST_F(PudPlainRootActivatorTest, TheRootStoresItsAxiomsGoalsAsItsOwnAxiom) {
    EXPECT_CALL(goal_lists, build(axiom.body)).WillOnce(Return(built));
    EXPECT_CALL(goals, store(pud_test_node(2), built));
    EXPECT_CALL(node_axioms, store(pud_test_node(2), pud_test_node(2)));
    activator.activate_root(pud_test_node(2));
}

TEST_F(PudPlainRootActivatorTest, TheRootsLvcIsItsAxiomsVarCountAndItStartsUnbound) {
    EXPECT_CALL(lvcs, store(pud_test_node(2), 4u));
    EXPECT_CALL(bindings, store(pud_test_node(2), pud_ubs_state{}));
    activator.activate_root(pud_test_node(2));
}

TEST_F(PudPlainRootActivatorTest, ARootIsNeverRefutedOnArrival) {
    EXPECT_FALSE(activator.activate_root(pud_test_node(2)));
}
