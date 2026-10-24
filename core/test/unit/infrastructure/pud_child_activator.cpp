#include <optional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_child_activator.hpp"
#include "value_objects/pud_inference.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

using queries_t = immer::map<pud_body_goal_id, pud_mhq_head_id>;

struct MockGetActiveState {
    MOCK_METHOD(const pud_active_state&, get, (pud_global_node_id));
};
struct MockStoreActiveState {
    MOCK_METHOD(void, store, (pud_global_node_id, pud_active_state));
};
struct MockGetInference {
    MOCK_METHOD(const pud_inference&, get_inference, (pud_global_node_id));
};
struct MockGetNodeGoals {
    MOCK_METHOD(const pud_goal_list&, get, (pud_global_node_id));
};
struct MockFindNodeGoal {
    MOCK_METHOD(const expr*, find, (pud_global_node_id, pud_body_goal_id));
};
struct MockStoreNodeGoals {
    MOCK_METHOD(void, store, (pud_global_node_id, pud_goal_list));
};
struct MockIncrementLiveChildCount {
    MOCK_METHOD(void, increment, (pud_global_node_id));
};
struct MockBind {
    MOCK_METHOD(pud_ubs_state, bind, (const pud_ubs_state&, pud_global_node_id));
};
struct MockWatchDescend {
    MOCK_METHOD(pud_structure_watch_change, descend,
        (const pud_structure_watch&, const pud_ubs_state&, pud_global_node_id, pud_body_goal_id, const pud_goal_list&));
};
struct MockForkQuery {
    MOCK_METHOD(std::optional<pud_mhq_head_id>, try_fork, (pud_mhq_head_id, const pud_ubs_state&, const expr*, uint32_t));
};
struct MockOpenQuery {
    MOCK_METHOD(std::optional<pud_mhq_head_id>, try_open, (pud_module_id, const pud_ubs_state&, const expr*, uint32_t));
};
struct MockHoldQuery {
    MOCK_METHOD(void, hold, (pud_mhq_head_id, pud_global_node_id, pud_body_goal_id));
};
struct MockCheckQueryExhausted {
    MOCK_METHOD(bool, check_exhausted, (pud_mhq_head_id));
};

using test_child_activator_t = pud_child_activator<
    NiceMock<MockGetActiveState>, NiceMock<MockStoreActiveState>, NiceMock<MockGetInference>,
    NiceMock<MockGetNodeGoals>, NiceMock<MockFindNodeGoal>, NiceMock<MockStoreNodeGoals>,
    NiceMock<MockIncrementLiveChildCount>, NiceMock<MockBind>, NiceMock<MockWatchDescend>, NiceMock<MockForkQuery>,
    NiceMock<MockOpenQuery>, NiceMock<MockHoldQuery>, NiceMock<MockCheckQueryExhausted>>;

// The parent, node 3 at lvc 5, has goals a and b, holds queries 40 and 41 for them, and the child,
// node 4, was resolved on goal 0. The child keeps goal 1, adds goal 2 of its own, and two variables.
struct PudChildActivatorTest : public ::testing::Test {
    NiceMock<MockGetActiveState> get_states;
    NiceMock<MockStoreActiveState> store_states;
    NiceMock<MockGetInference> inferences;
    NiceMock<MockGetNodeGoals> get_goals;
    NiceMock<MockFindNodeGoal> find_goal;
    NiceMock<MockStoreNodeGoals> store_goals;
    NiceMock<MockIncrementLiveChildCount> live_child_counts;
    NiceMock<MockBind> binder;
    NiceMock<MockWatchDescend> watcher;
    NiceMock<MockForkQuery> forker;
    NiceMock<MockOpenQuery> opener;
    NiceMock<MockHoldQuery> holders;
    NiceMock<MockCheckQueryExhausted> exhausted;
    test_child_activator_t activator{
        get_states, store_states, inferences, get_goals, find_goal, store_goals, live_child_counts, binder, watcher,
        forker, opener, holders, exhausted};
    expr goal_a{expr::functor{1, {}}};
    expr goal_b{expr::functor{2, {}}};
    expr goal_c{expr::functor{3, {}}};
    pud_goal_list parent_goals = pud_goal_list{.goals = {}, .next_id = 0}.adding(&goal_a).adding(&goal_b);
    pud_goal_list child_goals = parent_goals.closing(0).adding(&goal_c);
    pud_structure_watch parent_watch;
    pud_structure_watch child_watch;
    pud_ubs_state child_bindings;
    pud_active_state parent_state;
    pud_inference child_inference{.parent = 3, .call_site = 0, .substitutions = {}, .body_goals = {&goal_c}, .var_count = 2};

    pud_structure_watch_change changing(std::vector<pud_body_goal_id> changed_goals) {
        return pud_structure_watch_change{.watch = child_watch, .changed_goals = std::move(changed_goals)};
    }

    void SetUp() override {
        parent_watch.goal_to_reps = parent_watch.goal_to_reps.set(1, {});
        child_watch.goal_to_reps = child_watch.goal_to_reps.set(2, {});
        child_bindings.parents = child_bindings.parents.set(5, 1);
        parent_state = pud_active_state{
            .lvc = 5, .bindings = {}, .watch = parent_watch, .queries = queries_t{}.set(0, 40).set(1, 41)};
        ON_CALL(get_states, get(pud_test_node(3))).WillByDefault(ReturnRef(parent_state));
        ON_CALL(inferences, get_inference(pud_test_node(4))).WillByDefault(ReturnRef(child_inference));
        ON_CALL(get_goals, get(pud_test_node(3))).WillByDefault(ReturnRef(parent_goals));
        ON_CALL(find_goal, find(pud_test_node(4), 1)).WillByDefault(Return(&goal_b));
        ON_CALL(binder, bind(_, pud_test_node(4))).WillByDefault(Return(child_bindings));
        ON_CALL(watcher, descend(_, _, _, _, _)).WillByDefault(Return(changing({})));
        ON_CALL(opener, try_open(k_test_module, _, &goal_c, 7)).WillByDefault(Return(70));
        ON_CALL(forker, try_fork(41, _, &goal_b, 7)).WillByDefault(Return(60));
        ON_CALL(exhausted, check_exhausted(_)).WillByDefault(Return(false));
    }
};

}

TEST_F(PudChildActivatorTest, AChildsGoalsAreItsParentsButTheCallSiteThenItsOwn) {
    EXPECT_CALL(store_goals, store(pud_test_node(4), child_goals));
    activator.activate_child(pud_test_node(3), pud_test_node(4));
}

TEST_F(PudChildActivatorTest, AChildCountsAsOneMoreLiveChildOfItsParent) {
    EXPECT_CALL(live_child_counts, increment(pud_test_node(3)));
    activator.activate_child(pud_test_node(3), pud_test_node(4));
}

TEST_F(PudChildActivatorTest, AChildsGoalsAreStoredBeforeAnyOfItsQueriesIsForkedOrOpened) {
    ON_CALL(watcher, descend(_, _, _, _, _)).WillByDefault(Return(changing({1})));
    InSequence in_order;
    EXPECT_CALL(store_goals, store(pud_test_node(4), child_goals));
    EXPECT_CALL(forker, try_fork(_, _, _, _));
    EXPECT_CALL(opener, try_open(_, _, _, _));
    activator.activate_child(pud_test_node(3), pud_test_node(4));
}

TEST_F(PudChildActivatorTest, AChildKeepsItsParentsQueryForAnUnchangedGoalAndOpensOneForItsOwn) {
    EXPECT_CALL(watcher, descend(parent_watch, child_bindings, pud_test_node(4), 0u, child_goals));
    EXPECT_CALL(forker, try_fork(_, _, _, _)).Times(0);
    EXPECT_CALL(opener, try_open(k_test_module, child_bindings, &goal_c, 7u)).WillOnce(Return(70));
    EXPECT_CALL(holders, hold(41u, pud_test_node(4), 1u));
    EXPECT_CALL(holders, hold(70u, pud_test_node(4), 2u));
    EXPECT_CALL(store_states, store(pud_test_node(4), pud_active_state{
        .lvc = 7, .bindings = child_bindings, .watch = child_watch, .queries = queries_t{}.set(1, 41).set(2, 70)}));
    EXPECT_FALSE(activator.activate_child(pud_test_node(3), pud_test_node(4)));
}

TEST_F(PudChildActivatorTest, AChildsLvcIsItsParentsPlusItsOwnVariables) {
    EXPECT_CALL(store_states, store(pud_test_node(4), ::testing::Field(&pud_active_state::lvc, 7u)));
    activator.activate_child(pud_test_node(3), pud_test_node(4));
}

TEST_F(PudChildActivatorTest, AGoalThatChangedStructureForksTheParentsQueryInTheChildsEnvironment) {
    ON_CALL(watcher, descend(_, _, _, _, _)).WillByDefault(Return(changing({1})));
    EXPECT_CALL(forker, try_fork(41, child_bindings, &goal_b, 7u)).WillOnce(Return(60));
    EXPECT_CALL(holders, hold(60u, pud_test_node(4), 1u));
    EXPECT_CALL(holders, hold(70u, pud_test_node(4), 2u));
    EXPECT_CALL(store_states, store(pud_test_node(4), pud_active_state{
        .lvc = 7, .bindings = child_bindings, .watch = child_watch, .queries = queries_t{}.set(1, 60).set(2, 70)}));
    EXPECT_FALSE(activator.activate_child(pud_test_node(3), pud_test_node(4)));
}

TEST_F(PudChildActivatorTest, AForkWithoutCandidatesDropsTheQueryAndRefutesTheChild) {
    ON_CALL(watcher, descend(_, _, _, _, _)).WillByDefault(Return(changing({1})));
    ON_CALL(forker, try_fork(_, _, _, _)).WillByDefault(Return(std::nullopt));
    EXPECT_CALL(holders, hold(70u, pud_test_node(4), 2u));
    EXPECT_CALL(store_states, store(pud_test_node(4), pud_active_state{
        .lvc = 7, .bindings = child_bindings, .watch = child_watch, .queries = queries_t{}.set(2, 70)}));
    EXPECT_TRUE(activator.activate_child(pud_test_node(3), pud_test_node(4)));
}

TEST_F(PudChildActivatorTest, AGoalOfItsOwnWithoutCandidatesRefutesTheChild) {
    ON_CALL(opener, try_open(_, _, _, _)).WillByDefault(Return(std::nullopt));
    EXPECT_CALL(holders, hold(41u, pud_test_node(4), 1u));
    EXPECT_TRUE(activator.activate_child(pud_test_node(3), pud_test_node(4)));
}

TEST_F(PudChildActivatorTest, AnInheritedQueryAlreadyExhaustedRefutesTheChildButIsStillHeld) {
    ON_CALL(exhausted, check_exhausted(41u)).WillByDefault(Return(true));
    EXPECT_CALL(holders, hold(41u, pud_test_node(4), 1u));
    EXPECT_CALL(holders, hold(70u, pud_test_node(4), 2u));
    EXPECT_TRUE(activator.activate_child(pud_test_node(3), pud_test_node(4)));
}

TEST_F(PudChildActivatorTest, AChangedGoalItsParentHoldsNoQueryForIsOpenedInTheChildsEnvironment) {
    parent_state.queries = queries_t{}.set(0, 40);
    ON_CALL(watcher, descend(_, _, _, _, _)).WillByDefault(Return(changing({1})));
    EXPECT_CALL(forker, try_fork(_, _, _, _)).Times(0);
    EXPECT_CALL(opener, try_open(k_test_module, child_bindings, &goal_b, 7u)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(opener, try_open(k_test_module, child_bindings, &goal_c, 7u)).WillOnce(Return(70));
    EXPECT_TRUE(activator.activate_child(pud_test_node(3), pud_test_node(4)));
}

TEST_F(PudChildActivatorTest, ARefutedChildStillCountsAsALiveChildUntilItIsRefuted) {
    ON_CALL(opener, try_open(_, _, _, _)).WillByDefault(Return(std::nullopt));
    EXPECT_CALL(live_child_counts, increment(pud_test_node(3)));
    EXPECT_TRUE(activator.activate_child(pud_test_node(3), pud_test_node(4)));
}
