#include <map>
#include <optional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_root_activator.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "value_objects/pud_node_queries.hpp"
#include "value_objects/pud_structure_watch.hpp"
#include "value_objects/pud_ubs_state.hpp"
#include "value_objects/rule.hpp"
#include "pud_test_node.hpp"
#include "pud_test_node_queries.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ResultOf;
using ::testing::ReturnRef;

namespace {

struct MockGetAxiom {
    MOCK_METHOD(const rule&, get_axiom, (pud_global_node_id));
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
struct MockStoreNodeWatch {
    MOCK_METHOD(void, store, (pud_global_node_id, pud_structure_watch));
};
struct MockRemoveQuery {
    MOCK_METHOD(void, remove, (pud_mhq_head_id));
};
using test_node_queries_t = pud_node_queries<NiceMock<MockRemoveQuery>>;
using heads_t = std::map<pud_body_goal_id, pud_mhq_head_id>;
struct MockStoreNodeQueries {
    MOCK_METHOD(void, store, (pud_global_node_id, test_node_queries_t));
};
struct MockWatchRoot {
    MOCK_METHOD(pud_structure_watch, watch_root, (const pud_ubs_state&, const pud_goal_list&, pud_module_id));
};
struct MockOpenQuery {
    MOCK_METHOD(std::optional<pud_mhq_head_id>, try_open, (pud_module_id, const pud_ubs_state&, const expr*, uint32_t));
};
struct MockIntroduceQuery {
    MOCK_METHOD(void, introduce, (pud_mhq_head_id, pud_global_node_id, pud_body_goal_id));
};

using test_root_activator_t = pud_root_activator<
    NiceMock<MockGetAxiom>, NiceMock<MockBuildRootGoalList>, NiceMock<MockStoreNodeGoals>, NiceMock<MockStoreNodeAxiom>,
    NiceMock<MockStoreNodeLvc>, NiceMock<MockStoreNodeBindings>, NiceMock<MockStoreNodeWatch>,
    NiceMock<MockStoreNodeQueries>, NiceMock<MockWatchRoot>, NiceMock<MockOpenQuery>, NiceMock<MockRemoveQuery>,
    NiceMock<MockIntroduceQuery>>;

// The axiom at node 2 has the body a, b and three variables.
struct PudRootActivatorTest : public ::testing::Test {
    NiceMock<MockRemoveQuery> remover;
    NiceMock<MockGetAxiom> axioms;
    NiceMock<MockBuildRootGoalList> root_goal_lists;
    NiceMock<MockStoreNodeGoals> goals;
    NiceMock<MockStoreNodeAxiom> store_node_axiom;
    NiceMock<MockStoreNodeLvc> store_lvcs;
    NiceMock<MockStoreNodeBindings> store_bindings;
    NiceMock<MockStoreNodeWatch> store_watches;
    NiceMock<MockStoreNodeQueries> store_queries;
    NiceMock<MockWatchRoot> watcher;
    NiceMock<MockOpenQuery> queries;
    NiceMock<MockIntroduceQuery> introducers;
    test_root_activator_t activator{
        axioms, root_goal_lists, goals, store_node_axiom, store_lvcs, store_bindings, store_watches, store_queries,
        watcher, queries, remover, introducers};
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
        ON_CALL(root_goal_lists, build(axiom.body)).WillByDefault(Return(root_goals));
        ON_CALL(watcher, watch_root(_, _, _)).WillByDefault(Return(root_watch));
        ON_CALL(queries, try_open(k_test_module, pud_ubs_state{}, &goal_a, 3)).WillByDefault(Return(50));
        ON_CALL(queries, try_open(k_test_module, pud_ubs_state{}, &goal_b, 3)).WillByDefault(Return(51));
    }
};

}

TEST_F(PudRootActivatorTest, AnAxiomsRootAxiomIsItselfAndIsStoredWithItsGoals) {
    EXPECT_CALL(store_node_axiom, store(pud_test_node(2), pud_test_node(2)));
    activator.activate_root(pud_test_node(2));
}

TEST_F(PudRootActivatorTest, AnAxiomsGoalsAreItsBodyInOrderAndAreStoredBeforeAnyQueryIsOpened) {
    InSequence in_order;
    EXPECT_CALL(root_goal_lists, build(axiom.body)).WillOnce(Return(root_goals));
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
    EXPECT_CALL(watcher, watch_root(pud_ubs_state{}, root_goals, _));
    activator.activate_root(pud_test_node(2));
}

TEST_F(PudRootActivatorTest, AnAxiomIntroducesAQueryPerGoalAndStoresItsActiveFieldsAtItsLvc) {
    EXPECT_CALL(introducers, introduce(50u, pud_test_node(2), 0u));
    EXPECT_CALL(introducers, introduce(51u, pud_test_node(2), 1u));
    EXPECT_CALL(store_lvcs, store(pud_test_node(2), 3u));
    EXPECT_CALL(store_bindings, store(pud_test_node(2), pud_ubs_state{}));
    EXPECT_CALL(store_watches, store(pud_test_node(2), root_watch));
    EXPECT_CALL(store_queries, store(
        pud_test_node(2), ResultOf(&heads_of<NiceMock<MockRemoveQuery>>, heads_t{{0, 50}, {1, 51}})));
    EXPECT_FALSE(activator.activate_root(pud_test_node(2)));
}

TEST_F(PudRootActivatorTest, TheActiveFieldsAreStoredInOrderAfterTheQueriesAreIntroduced) {
    InSequence order;
    EXPECT_CALL(introducers, introduce(_, _, _)).Times(2);
    EXPECT_CALL(store_lvcs, store(pud_test_node(2), _));
    EXPECT_CALL(store_bindings, store(pud_test_node(2), _));
    EXPECT_CALL(store_watches, store(pud_test_node(2), _));
    EXPECT_CALL(store_queries, store(pud_test_node(2), _));
    activator.activate_root(pud_test_node(2));
}

TEST_F(PudRootActivatorTest, AGoalWithoutCandidatesRefutesTheAxiomButTheOtherQueriesAreStillIntroduced) {
    ON_CALL(queries, try_open(_, _, &goal_a, _)).WillByDefault(Return(std::nullopt));
    EXPECT_CALL(introducers, introduce(51u, pud_test_node(2), 1u));
    EXPECT_CALL(store_queries, store(
        pud_test_node(2), ResultOf(&heads_of<NiceMock<MockRemoveQuery>>, heads_t{{1, 51}})));
    EXPECT_TRUE(activator.activate_root(pud_test_node(2)));
}

TEST_F(PudRootActivatorTest, AnAxiomWithoutABodyHasNoGoalsAndOpensNothing) {
    rule fact{&head, {}, 0};
    ON_CALL(axioms, get_axiom(pud_test_node(2))).WillByDefault(ReturnRef(fact));
    EXPECT_CALL(root_goal_lists, build(fact.body)).WillOnce(Return(pud_goal_list{.goals = {}, .next_id = 0}));
    EXPECT_CALL(goals, store(pud_test_node(2), (pud_goal_list{.goals = {}, .next_id = 0})));
    EXPECT_CALL(queries, try_open(_, _, _, _)).Times(0);
    EXPECT_FALSE(activator.activate_root(pud_test_node(2)));
}
