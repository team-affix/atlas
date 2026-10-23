#include <optional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_child_activator.hpp"
#include "value_objects/pud_node_delta.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
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
struct MockBind {
    MOCK_METHOD(pud_ubs_state, bind, (const pud_ubs_state&, pud_global_node_id));
};
struct MockWatchDescend {
    MOCK_METHOD(pud_structure_watch_change, descend,
        (const pud_structure_watch&, const pud_ubs_state&, pud_global_node_id, pud_body_goal_id, const pud_goal_list&));
};
struct MockGetNodeGoals {
    MOCK_METHOD(const pud_goal_list&, get, (pud_global_node_id));
};
struct MockGetInference {
    MOCK_METHOD(const pud_node_delta&, get_inference, (pud_global_node_id));
};
struct MockForkQuery {
    MOCK_METHOD(std::optional<pud_mhq_head_id>, fork, (pud_mhq_head_id, pud_global_node_id, const pud_ubs_state&, pud_body_goal_id));
};
struct MockOpenQuery {
    MOCK_METHOD(std::optional<pud_mhq_head_id>, open, (pud_global_node_id, const pud_ubs_state&, pud_body_goal_id));
};
struct MockHoldQuery {
    MOCK_METHOD(void, hold, (pud_mhq_head_id, pud_global_node_id, pud_body_goal_id));
};
struct MockCheckQueryExhausted {
    MOCK_METHOD(bool, check_exhausted, (pud_mhq_head_id));
};

using test_child_activator_t = pud_child_activator<
    NiceMock<MockGetActiveState>, NiceMock<MockStoreActiveState>, NiceMock<MockBind>, NiceMock<MockWatchDescend>,
    NiceMock<MockGetNodeGoals>, NiceMock<MockGetInference>, NiceMock<MockForkQuery>, NiceMock<MockOpenQuery>,
    NiceMock<MockHoldQuery>, NiceMock<MockCheckQueryExhausted>>;

// The parent holds queries 40 and 41 for goals 0 and 1 and was resolved on goal 0. The child keeps
// goal 1 and adds goal 2 of its own.
struct PudChildActivatorTest : public ::testing::Test {
    NiceMock<MockGetActiveState> get_states;
    NiceMock<MockStoreActiveState> store_states;
    NiceMock<MockBind> binder;
    NiceMock<MockWatchDescend> watcher;
    NiceMock<MockGetNodeGoals> goals;
    NiceMock<MockGetInference> inferences;
    NiceMock<MockForkQuery> forker;
    NiceMock<MockOpenQuery> opener;
    NiceMock<MockHoldQuery> holders;
    NiceMock<MockCheckQueryExhausted> exhausted;
    test_child_activator_t activator{
        get_states, store_states, binder, watcher, goals, inferences, forker, opener, holders, exhausted};
    expr goal_a{expr::functor{1, {}}};
    expr goal_b{expr::functor{2, {}}};
    expr goal_c{expr::functor{3, {}}};
    pud_goal_list child_goals =
        pud_goal_list{.goals = {}, .next_id = 0}.adding(&goal_a).adding(&goal_b).closing(0).adding(&goal_c);
    pud_structure_watch parent_watch;
    pud_structure_watch child_watch;
    pud_ubs_state child_bindings;
    pud_active_state parent_state;
    pud_node_delta child_inference{.parent = 3, .call_site = 0, .substitutions = {}, .body_goals = {&goal_c}, .var_count = 0};

    pud_structure_watch_change changing(std::vector<pud_body_goal_id> changed_goals) {
        return pud_structure_watch_change{.watch = child_watch, .changed_goals = std::move(changed_goals)};
    }

    void SetUp() override {
        parent_watch.goal_to_reps = parent_watch.goal_to_reps.set(1, {});
        child_watch.goal_to_reps = child_watch.goal_to_reps.set(2, {});
        child_bindings.parents = child_bindings.parents.set(5, 1);
        parent_state = pud_active_state{.bindings = {}, .watch = parent_watch, .queries = queries_t{}.set(0, 40).set(1, 41)};
        ON_CALL(get_states, get(pud_test_node(3))).WillByDefault(ReturnRef(parent_state));
        ON_CALL(inferences, get_inference(pud_test_node(4))).WillByDefault(ReturnRef(child_inference));
        ON_CALL(binder, bind(_, pud_test_node(4))).WillByDefault(Return(child_bindings));
        ON_CALL(goals, get(pud_test_node(4))).WillByDefault(ReturnRef(child_goals));
        ON_CALL(watcher, descend(_, _, _, _, _)).WillByDefault(Return(changing({})));
        ON_CALL(opener, open(pud_test_node(4), _, 2)).WillByDefault(Return(70));
        ON_CALL(forker, fork(41, pud_test_node(4), _, 1)).WillByDefault(Return(60));
        ON_CALL(exhausted, check_exhausted(_)).WillByDefault(Return(false));
    }
};

}

TEST_F(PudChildActivatorTest, AChildKeepsItsParentsQueryForAnUnchangedGoalAndOpensOneForItsOwn) {
    EXPECT_CALL(watcher, descend(parent_watch, child_bindings, pud_test_node(4), 0u, _));
    EXPECT_CALL(forker, fork(_, _, _, _)).Times(0);
    EXPECT_CALL(opener, open(pud_test_node(4), child_bindings, 2)).WillOnce(Return(70));
    EXPECT_CALL(holders, hold(41u, pud_test_node(4), 1u));
    EXPECT_CALL(holders, hold(70u, pud_test_node(4), 2u));
    EXPECT_CALL(store_states, store(pud_test_node(4), pud_active_state{.bindings = child_bindings, .watch = child_watch, .queries = queries_t{}.set(1, 41).set(2, 70)}));
    EXPECT_FALSE(activator.activate_child(pud_test_node(3), pud_test_node(4)));
}

TEST_F(PudChildActivatorTest, AGoalThatChangedStructureForksTheParentsQueryInTheChildsEnvironment) {
    ON_CALL(watcher, descend(_, _, _, _, _)).WillByDefault(Return(changing({1})));
    EXPECT_CALL(forker, fork(41, pud_test_node(4), child_bindings, 1)).WillOnce(Return(60));
    EXPECT_CALL(holders, hold(60u, pud_test_node(4), 1u));
    EXPECT_CALL(holders, hold(70u, pud_test_node(4), 2u));
    EXPECT_CALL(store_states, store(pud_test_node(4), pud_active_state{.bindings = child_bindings, .watch = child_watch, .queries = queries_t{}.set(1, 60).set(2, 70)}));
    EXPECT_FALSE(activator.activate_child(pud_test_node(3), pud_test_node(4)));
}

TEST_F(PudChildActivatorTest, AForkWithoutCandidatesDropsTheQueryAndRefutesTheChild) {
    ON_CALL(watcher, descend(_, _, _, _, _)).WillByDefault(Return(changing({1})));
    ON_CALL(forker, fork(_, _, _, _)).WillByDefault(Return(std::nullopt));
    EXPECT_CALL(holders, hold(70u, pud_test_node(4), 2u));
    EXPECT_CALL(store_states, store(pud_test_node(4), pud_active_state{.bindings = child_bindings, .watch = child_watch, .queries = queries_t{}.set(2, 70)}));
    EXPECT_TRUE(activator.activate_child(pud_test_node(3), pud_test_node(4)));
}

TEST_F(PudChildActivatorTest, AGoalOfItsOwnWithoutCandidatesRefutesTheChild) {
    ON_CALL(opener, open(_, _, _)).WillByDefault(Return(std::nullopt));
    EXPECT_CALL(holders, hold(41u, pud_test_node(4), 1u));
    EXPECT_TRUE(activator.activate_child(pud_test_node(3), pud_test_node(4)));
}

TEST_F(PudChildActivatorTest, AnInheritedQueryAlreadyExhaustedRefutesTheChildButIsStillHeld) {
    ON_CALL(exhausted, check_exhausted(41u)).WillByDefault(Return(true));
    EXPECT_CALL(holders, hold(41u, pud_test_node(4), 1u));
    EXPECT_CALL(holders, hold(70u, pud_test_node(4), 2u));
    EXPECT_TRUE(activator.activate_child(pud_test_node(3), pud_test_node(4)));
}
