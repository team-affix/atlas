#include <optional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_root_activator.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "value_objects/rule.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

using queries_t = immer::map<pud_body_goal_id, pud_mhq_head_id>;

struct MockGetAxiom {
    MOCK_METHOD(const rule&, get_axiom, (pud_global_node_id));
};
struct MockOpenGoals {
    MOCK_METHOD(pud_goal_list, open_goals, ((const std::vector<const expr*>&)), (const));
};
struct MockStoreNodeGoals {
    MOCK_METHOD(void, store, (pud_global_node_id, pud_goal_list));
};
struct MockStoreActiveState {
    MOCK_METHOD(void, store, (pud_global_node_id, pud_active_state));
};
struct MockWatchRoot {
    MOCK_METHOD(pud_structure_watch, watch_root, (const pud_ubs_state&, const pud_goal_list&));
};
struct MockOpenQuery {
    MOCK_METHOD(std::optional<pud_mhq_head_id>, try_open, (pud_module_id, const pud_ubs_state&, const expr*, uint32_t));
};
struct MockHoldQuery {
    MOCK_METHOD(void, hold, (pud_mhq_head_id, pud_global_node_id, pud_body_goal_id));
};

using test_root_activator_t = pud_root_activator<
    NiceMock<MockGetAxiom>, NiceMock<MockOpenGoals>, NiceMock<MockStoreNodeGoals>, NiceMock<MockStoreActiveState>,
    NiceMock<MockWatchRoot>, NiceMock<MockOpenQuery>, NiceMock<MockHoldQuery>>;

// The axiom at node 2 has the body a, b and three variables.
struct PudRootActivatorTest : public ::testing::Test {
    NiceMock<MockGetAxiom> axioms;
    NiceMock<MockOpenGoals> open_goals;
    NiceMock<MockStoreNodeGoals> goals;
    NiceMock<MockStoreActiveState> store_states;
    NiceMock<MockWatchRoot> watcher;
    NiceMock<MockOpenQuery> queries;
    NiceMock<MockHoldQuery> holders;
    test_root_activator_t activator{axioms, open_goals, goals, store_states, watcher, queries, holders};
    expr head{expr::functor{9, {}}};
    expr goal_a{expr::functor{1, {}}};
    expr goal_b{expr::functor{2, {}}};
    rule axiom{&head, {&goal_a, &goal_b}, 3};
    pud_goal_list root_goals{
        .goals = immer::map<pud_body_goal_id, const expr*>{}.set(0, &goal_a).set(1, &goal_b), .next_id = 2};
    pud_structure_watch root_watch;

    void SetUp() override {
        root_watch.goal_to_reps = root_watch.goal_to_reps.set(0, {});
        ON_CALL(axioms, get_axiom(pud_test_node(2))).WillByDefault(ReturnRef(axiom));
        ON_CALL(open_goals, open_goals(axiom.body)).WillByDefault(Return(root_goals));
        ON_CALL(watcher, watch_root(_, _)).WillByDefault(Return(root_watch));
        ON_CALL(queries, try_open(k_test_module, pud_ubs_state{}, &goal_a, 3)).WillByDefault(Return(50));
        ON_CALL(queries, try_open(k_test_module, pud_ubs_state{}, &goal_b, 3)).WillByDefault(Return(51));
    }
};

}

TEST_F(PudRootActivatorTest, AnAxiomsGoalsAreItsBodyInOrderAndAreStoredBeforeAnyQueryIsOpened) {
    InSequence in_order;
    EXPECT_CALL(open_goals, open_goals(axiom.body)).WillOnce(Return(root_goals));
    EXPECT_CALL(goals, store(pud_test_node(2), root_goals));
    EXPECT_CALL(queries, try_open(_, _, _, _)).Times(2);
    activator.activate_root(pud_test_node(2));
}

TEST_F(PudRootActivatorTest, EachGoalsQueryIsOpenedInTheAxiomsModuleFromEmptyBindingsAtItsLvc) {
    EXPECT_CALL(queries, try_open(k_test_module, pud_ubs_state{}, &goal_a, 3u));
    EXPECT_CALL(queries, try_open(k_test_module, pud_ubs_state{}, &goal_b, 3u));
    activator.activate_root(pud_test_node(2));
}

TEST_F(PudRootActivatorTest, TheWatchCoversTheAxiomsGoals) {
    EXPECT_CALL(watcher, watch_root(pud_ubs_state{}, root_goals));
    activator.activate_root(pud_test_node(2));
}

TEST_F(PudRootActivatorTest, AnAxiomHoldsAQueryPerGoalAndStoresItsStateAtItsLvc) {
    EXPECT_CALL(holders, hold(50u, pud_test_node(2), 0u));
    EXPECT_CALL(holders, hold(51u, pud_test_node(2), 1u));
    EXPECT_CALL(store_states, store(pud_test_node(2), pud_active_state{
        .lvc = 3, .bindings = {}, .watch = root_watch, .queries = queries_t{}.set(0, 50).set(1, 51)}));
    EXPECT_FALSE(activator.activate_root(pud_test_node(2)));
}

TEST_F(PudRootActivatorTest, AGoalWithoutCandidatesRefutesTheAxiomButTheOtherQueriesAreStillHeld) {
    ON_CALL(queries, try_open(_, _, &goal_a, _)).WillByDefault(Return(std::nullopt));
    EXPECT_CALL(holders, hold(51u, pud_test_node(2), 1u));
    EXPECT_CALL(store_states, store(pud_test_node(2), pud_active_state{
        .lvc = 3, .bindings = {}, .watch = root_watch, .queries = queries_t{}.set(1, 51)}));
    EXPECT_TRUE(activator.activate_root(pud_test_node(2)));
}

TEST_F(PudRootActivatorTest, AnAxiomWithoutABodyHasNoGoalsAndOpensNothing) {
    rule fact{&head, {}, 0};
    ON_CALL(axioms, get_axiom(pud_test_node(2))).WillByDefault(ReturnRef(fact));
    EXPECT_CALL(open_goals, open_goals(fact.body)).WillOnce(Return(pud_goal_list{.goals = {}, .next_id = 0}));
    EXPECT_CALL(goals, store(pud_test_node(2), (pud_goal_list{.goals = {}, .next_id = 0})));
    EXPECT_CALL(queries, try_open(_, _, _, _)).Times(0);
    EXPECT_FALSE(activator.activate_root(pud_test_node(2)));
}
