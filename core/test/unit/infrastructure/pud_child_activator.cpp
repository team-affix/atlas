#include <map>
#include <optional>
#include <stdexcept>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_child_activator.hpp"
#include "value_objects/pud_node_queries.hpp"
#include "value_objects/pud_inference.hpp"
#include "value_objects/pud_query_introduction.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "pud_test_node.hpp"
#include "pud_test_node_queries.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::ResultOf;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetNodeLvc {
    MOCK_METHOD(uint32_t, get, (pud_global_node_id));
};
struct MockGetNodeBindings {
    MOCK_METHOD(const pud_ubs_state&, get, (pud_global_node_id));
};
struct MockGetNodeWatch {
    MOCK_METHOD(const pud_structure_watch&, get, (pud_global_node_id));
};
struct MockRemoveQuery {
    MOCK_METHOD(void, remove, (pud_mhq_head_id));
};
using test_node_queries_t = pud_node_queries<NiceMock<MockRemoveQuery>>;
using test_owner_box_t = immer::box<pud_mhq_head_owner<NiceMock<MockRemoveQuery>>>;
using heads_t = std::map<pud_body_goal_id, pud_mhq_head_id>;
struct MockGetNodeQueries {
    MOCK_METHOD(const test_node_queries_t&, get, (pud_global_node_id));
};
struct MockStoreNodeLvc {
    MOCK_METHOD(void, store, (pud_global_node_id, uint32_t));
};
struct MockStoreNodeBindings {
    MOCK_METHOD(void, store, (pud_global_node_id, pud_ubs_state));
};
struct MockStoreNodeWatch {
    MOCK_METHOD(void, store, (pud_global_node_id, pud_structure_watch));
};
struct MockStoreNodeQueries {
    MOCK_METHOD(void, store, (pud_global_node_id, test_node_queries_t));
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
struct MockWatchDescend {
    MOCK_METHOD(pud_structure_watch_change, descend,
        (const pud_structure_watch&, const pud_ubs_state&, pud_global_node_id, const pud_goal_list&));
};
struct MockForkQuery {
    MOCK_METHOD(std::optional<pud_mhq_head_id>, try_fork, (pud_mhq_head_id, const pud_ubs_state&, const expr*, uint32_t));
};
struct MockOpenQuery {
    MOCK_METHOD(std::optional<pud_mhq_head_id>, try_open, (pud_module_id, const pud_ubs_state&, const expr*, uint32_t));
};
struct MockIntroduceQuery {
    MOCK_METHOD(void, introduce, (pud_mhq_head_id, pud_global_node_id, pud_body_goal_id));
};
using exhausted_t = std::map<pud_mhq_head_id, pud_query_introduction>;
struct MockGetExhaustedQueries {
    MOCK_METHOD(const exhausted_t&, exhausted, (), (const));
};

using test_child_activator_t = pud_child_activator<
    NiceMock<MockGetNodeLvc>, NiceMock<MockGetNodeBindings>, NiceMock<MockGetNodeWatch>, NiceMock<MockGetNodeQueries>,
    NiceMock<MockStoreNodeLvc>, NiceMock<MockStoreNodeBindings>, NiceMock<MockStoreNodeWatch>,
    NiceMock<MockStoreNodeQueries>, NiceMock<MockGetInference>,
    NiceMock<MockGetNodeGoals>, NiceMock<MockGetNodeAxiom>, NiceMock<MockBuildChildGoalList>,
    NiceMock<MockStoreNodeGoals>, NiceMock<MockStoreNodeAxiom>, NiceMock<MockIncrementLiveChildCount>,
    NiceMock<MockBind>, NiceMock<MockWatchDescend>, NiceMock<MockForkQuery>, NiceMock<MockOpenQuery>,
    NiceMock<MockRemoveQuery>, NiceMock<MockIntroduceQuery>, NiceMock<MockGetExhaustedQueries>>;

// The parent, node 3 at lvc 5, has goals a and b, holds queries 40 and 41 for them, and the child,
// node 4, was resolved on goal 0. The child keeps goal 1, adds goal 2 of its own, and two variables.
struct PudChildActivatorTest : public ::testing::Test {
    NiceMock<MockRemoveQuery> remover;
    NiceMock<MockGetNodeLvc> get_lvcs;
    NiceMock<MockGetNodeBindings> get_bindings;
    NiceMock<MockGetNodeWatch> get_watches;
    NiceMock<MockGetNodeQueries> get_queries;
    NiceMock<MockStoreNodeLvc> store_lvcs;
    NiceMock<MockStoreNodeBindings> store_bindings;
    NiceMock<MockStoreNodeWatch> store_watches;
    NiceMock<MockStoreNodeQueries> store_queries;
    NiceMock<MockGetInference> inferences;
    NiceMock<MockGetNodeGoals> get_goals;
    NiceMock<MockGetNodeAxiom> get_node_axiom;
    NiceMock<MockBuildChildGoalList> child_goal_lists;
    NiceMock<MockStoreNodeGoals> store_goals;
    NiceMock<MockStoreNodeAxiom> store_node_axiom;
    NiceMock<MockIncrementLiveChildCount> live_child_counts;
    NiceMock<MockBind> binder;
    NiceMock<MockWatchDescend> watcher;
    NiceMock<MockForkQuery> forker;
    NiceMock<MockOpenQuery> opener;
    NiceMock<MockIntroduceQuery> introducers;
    NiceMock<MockGetExhaustedQueries> exhausted_queries;
    test_child_activator_t activator{
        get_lvcs, get_bindings, get_watches, get_queries, store_lvcs, store_bindings, store_watches, store_queries,
        inferences, get_goals, get_node_axiom, child_goal_lists, store_goals,
        store_node_axiom, live_child_counts, binder, watcher, forker, opener, remover, introducers, exhausted_queries};
    expr goal_a{expr::functor{1, {}}};
    expr goal_b{expr::functor{2, {}}};
    expr goal_c{expr::functor{3, {}}};
    pud_goal_list parent_goals{
        .goals = immer::map<pud_body_goal_id, const expr*>{}.set(0, &goal_a).set(1, &goal_b), .next_id = 2};
    pud_goal_list child_goals{
        .goals = immer::map<pud_body_goal_id, const expr*>{}.set(1, &goal_b).set(2, &goal_c), .next_id = 3};
    pud_structure_watch parent_watch;
    pud_structure_watch child_watch;
    pud_ubs_state child_bindings;
    pud_ubs_state parent_bindings;
    test_node_queries_t parent_queries{
        test_node_queries_t{}.set(0, test_owner_box_t{remover, 40}).set(1, test_owner_box_t{remover, 41})};
    pud_inference child_inference{.parent = 3, .call_site = 0, .substitutions = {}, .body_goals = {&goal_c}, .var_count = 2};
    exhausted_t exhausted;

    void SetUp() override {
        parent_watch.goal_to_reps = parent_watch.goal_to_reps.set(1, {});
        child_watch.goal_to_reps = child_watch.goal_to_reps.set(2, {});
        child_bindings.parents = child_bindings.parents.set(5, 1);
        ON_CALL(get_lvcs, get(pud_test_node(3))).WillByDefault(Return(5u));
        ON_CALL(get_bindings, get(pud_test_node(3))).WillByDefault(ReturnRef(parent_bindings));
        ON_CALL(get_watches, get(pud_test_node(3))).WillByDefault(ReturnRef(parent_watch));
        ON_CALL(get_queries, get(pud_test_node(3))).WillByDefault(ReturnRef(parent_queries));
        ON_CALL(inferences, get_inference(pud_test_node(4))).WillByDefault(ReturnRef(child_inference));
        ON_CALL(get_goals, get(pud_test_node(3))).WillByDefault(ReturnRef(parent_goals));
        ON_CALL(child_goal_lists, build(parent_goals, 0u, child_inference.body_goals))
            .WillByDefault(Return(child_goals));
        ON_CALL(binder, bind(_, pud_test_node(4))).WillByDefault(Return(child_bindings));
        ON_CALL(watcher, descend(_, _, _, _))
            .WillByDefault(Return(pud_structure_watch_change{.watch = child_watch, .changed_goals = {}}));
        ON_CALL(opener, try_open(k_test_module, _, &goal_c, 7)).WillByDefault(Return(70));
        ON_CALL(forker, try_fork(41, _, &goal_b, 7)).WillByDefault(Return(60));
        ON_CALL(exhausted_queries, exhausted()).WillByDefault(ReturnRef(exhausted));
    }
};

}

TEST_F(PudChildActivatorTest, AChildInheritsItsParentsRootAxiom) {
    const pud_global_node_id parent = pud_test_node(3);
    const pud_global_node_id child = pud_test_node(4);
    const pud_global_node_id axiom = pud_test_node(2);
    EXPECT_CALL(get_node_axiom, get(parent)).WillOnce(Return(axiom));
    EXPECT_CALL(store_node_axiom, store(child, axiom));
    activator.activate_child(parent, child);
}

TEST_F(PudChildActivatorTest, AChildsGoalsAreItsParentsButTheCallSiteThenItsOwn) {
    EXPECT_CALL(child_goal_lists, build(parent_goals, 0u, child_inference.body_goals)).WillOnce(Return(child_goals));
    EXPECT_CALL(store_goals, store(pud_test_node(4), child_goals));
    activator.activate_child(pud_test_node(3), pud_test_node(4));
}

TEST_F(PudChildActivatorTest, AChildCountsAsOneMoreLiveChildOfItsParent) {
    EXPECT_CALL(live_child_counts, increment(pud_test_node(3)));
    activator.activate_child(pud_test_node(3), pud_test_node(4));
}

TEST_F(PudChildActivatorTest, AChildsGoalsAreStoredBeforeAnyOfItsQueriesIsForkedOrOpened) {
    ON_CALL(watcher, descend(_, _, _, _))
        .WillByDefault(Return(pud_structure_watch_change{.watch = child_watch, .changed_goals = {1}}));
    InSequence in_order;
    EXPECT_CALL(child_goal_lists, build(parent_goals, 0u, child_inference.body_goals)).WillOnce(Return(child_goals));
    EXPECT_CALL(store_goals, store(pud_test_node(4), child_goals));
    EXPECT_CALL(forker, try_fork(_, _, _, _));
    EXPECT_CALL(opener, try_open(_, _, _, _));
    activator.activate_child(pud_test_node(3), pud_test_node(4));
}

TEST_F(PudChildActivatorTest, AChildKeepsItsParentsQueryForAnUnchangedGoalAndIntroducesOneForItsOwn) {
    EXPECT_CALL(watcher, descend(parent_watch, child_bindings, pud_test_node(4), child_goals));
    EXPECT_CALL(forker, try_fork(_, _, _, _)).Times(0);
    EXPECT_CALL(opener, try_open(k_test_module, child_bindings, &goal_c, 7u)).WillOnce(Return(70));
    EXPECT_CALL(introducers, introduce(41u, _, _)).Times(0);
    EXPECT_CALL(introducers, introduce(70u, pud_test_node(4), 2u));
    EXPECT_CALL(store_bindings, store(pud_test_node(4), child_bindings));
    EXPECT_CALL(store_watches, store(pud_test_node(4), child_watch));
    EXPECT_CALL(store_queries, store(
        pud_test_node(4), ResultOf(&heads_of<NiceMock<MockRemoveQuery>>, heads_t{{1, 41}, {2, 70}})));
    EXPECT_FALSE(activator.activate_child(pud_test_node(3), pud_test_node(4)));
}

TEST_F(PudChildActivatorTest, TheActiveFieldsAreStoredInOrderAfterItsQueriesAreIntroduced) {
    InSequence order;
    EXPECT_CALL(introducers, introduce(_, _, _));
    EXPECT_CALL(store_lvcs, store(pud_test_node(4), _));
    EXPECT_CALL(store_bindings, store(pud_test_node(4), _));
    EXPECT_CALL(store_watches, store(pud_test_node(4), _));
    EXPECT_CALL(store_queries, store(pud_test_node(4), _));
    activator.activate_child(pud_test_node(3), pud_test_node(4));
}

TEST_F(PudChildActivatorTest, AChildsLvcIsItsParentsPlusItsOwnVariables) {
    EXPECT_CALL(store_lvcs, store(pud_test_node(4), 7u));
    activator.activate_child(pud_test_node(3), pud_test_node(4));
}

TEST_F(PudChildActivatorTest, AGoalThatChangedStructureForksTheParentsQueryInTheChildsEnvironment) {
    ON_CALL(watcher, descend(_, _, _, _))
        .WillByDefault(Return(pud_structure_watch_change{.watch = child_watch, .changed_goals = {1}}));
    EXPECT_CALL(forker, try_fork(41, child_bindings, &goal_b, 7u)).WillOnce(Return(60));
    EXPECT_CALL(introducers, introduce(60u, pud_test_node(4), 1u));
    EXPECT_CALL(introducers, introduce(70u, pud_test_node(4), 2u));
    EXPECT_CALL(store_queries, store(
        pud_test_node(4), ResultOf(&heads_of<NiceMock<MockRemoveQuery>>, heads_t{{1, 60}, {2, 70}})));
    EXPECT_FALSE(activator.activate_child(pud_test_node(3), pud_test_node(4)));
}

TEST_F(PudChildActivatorTest, AForkWithoutCandidatesDropsTheQueryAndRefutesTheChild) {
    ON_CALL(watcher, descend(_, _, _, _))
        .WillByDefault(Return(pud_structure_watch_change{.watch = child_watch, .changed_goals = {1}}));
    ON_CALL(forker, try_fork(_, _, _, _)).WillByDefault(Return(std::nullopt));
    EXPECT_CALL(introducers, introduce(70u, pud_test_node(4), 2u));
    EXPECT_CALL(store_queries, store(
        pud_test_node(4), ResultOf(&heads_of<NiceMock<MockRemoveQuery>>, heads_t{{2, 70}})));
    EXPECT_TRUE(activator.activate_child(pud_test_node(3), pud_test_node(4)));
}

TEST_F(PudChildActivatorTest, AGoalOfItsOwnWithoutCandidatesRefutesTheChild) {
    ON_CALL(opener, try_open(_, _, _, _)).WillByDefault(Return(std::nullopt));
    EXPECT_CALL(introducers, introduce(_, _, _)).Times(0);
    EXPECT_TRUE(activator.activate_child(pud_test_node(3), pud_test_node(4)));
}

TEST_F(PudChildActivatorTest, AnInheritedQueryAlreadyExhaustedRefutesTheChild) {
    exhausted.emplace(41, pud_query_introduction{.node = pud_test_node(1), .goal = 1});
    EXPECT_TRUE(activator.activate_child(pud_test_node(3), pud_test_node(4)));
}

TEST_F(PudChildActivatorTest, AnExhaustedQueryOnTheGoalTheChildClosedOrForkedLeavesItUnrefuted) {
    ON_CALL(watcher, descend(_, _, _, _))
        .WillByDefault(Return(pud_structure_watch_change{.watch = child_watch, .changed_goals = {1}}));
    exhausted.emplace(40, pud_query_introduction{.node = pud_test_node(1), .goal = 0});
    exhausted.emplace(41, pud_query_introduction{.node = pud_test_node(1), .goal = 1});
    EXPECT_FALSE(activator.activate_child(pud_test_node(3), pud_test_node(4)));
}

// A live parent holds a query for each of its goals (a failed open or fork refutes the node), so a
// goal it holds no query for can only be one the child added.
TEST_F(PudChildActivatorTest, AnInheritedGoalItsParentHoldsNoQueryForBreaksTheActivationInvariant) {
    parent_queries = test_node_queries_t{}.set(0, test_owner_box_t{remover, 40});
    EXPECT_THROW(activator.activate_child(pud_test_node(3), pud_test_node(4)), std::logic_error);
}

TEST_F(PudChildActivatorTest, ARefutedChildStillCountsAsALiveChildUntilItIsRefuted) {
    ON_CALL(opener, try_open(_, _, _, _)).WillByDefault(Return(std::nullopt));
    EXPECT_CALL(live_child_counts, increment(pud_test_node(3)));
    EXPECT_TRUE(activator.activate_child(pud_test_node(3), pud_test_node(4)));
}

TEST_F(PudChildActivatorTest, OnlyTheChildsOwnGoalsAreOpenedInIdOrder) {
    expr goal_d{expr::functor{4, {}}};
    child_inference.body_goals = {&goal_c, &goal_d};
    child_goals.goals = child_goals.goals.set(3, &goal_d);
    child_goals.next_id = 4;
    ON_CALL(child_goal_lists, build(parent_goals, 0u, child_inference.body_goals)).WillByDefault(Return(child_goals));
    InSequence order;
    EXPECT_CALL(opener, try_open(k_test_module, _, &goal_c, 7u)).WillOnce(Return(70));
    EXPECT_CALL(opener, try_open(k_test_module, _, &goal_d, 7u)).WillOnce(Return(71));
    EXPECT_FALSE(activator.activate_child(pud_test_node(3), pud_test_node(4)));
}
