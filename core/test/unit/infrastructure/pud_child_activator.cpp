#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_child_activator.hpp"
#include "value_objects/pud_goal_list.hpp"

using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

using queries_t = immer::map<pud_body_goal_id, pud_mhq_head_id>;

struct MockGetActiveState {
    MOCK_METHOD(const pud_active_state&, get, (pud_node_id));
};
struct MockStoreActiveState {
    MOCK_METHOD(void, store, (pud_node_id, pud_active_state));
};
struct MockBind {
    MOCK_METHOD(pud_ubs_state, bind, (const pud_ubs_state&, pud_node_id));
};
struct MockWatchDescend {
    MOCK_METHOD(pud_structure_watch_change, descend,
        (const pud_structure_watch&, const pud_ubs_state&, pud_node_id, pud_body_goal_id, const pud_goal_list&));
};
struct MockGetNodeGoals {
    MOCK_METHOD(const pud_goal_list&, get, (pud_node_id));
};
struct MockGetCallSite {
    MOCK_METHOD(pud_body_goal_id, get, (pud_node_id));
};
struct MockMakeChildQueries {
    MOCK_METHOD(pud_goal_queries, make_child_queries, (const queries_t&, pud_body_goal_id));
    MOCK_METHOD(void, admit_child_queries, (pud_goal_queries&, pud_node_id, const pud_ubs_state&, const queries_t&));
    MOCK_METHOD(void, fork_changed_goal, (pud_goal_queries&, pud_node_id, const pud_ubs_state&, pud_body_goal_id));
};

using test_child_activator_t = pud_child_activator<
    NiceMock<MockGetActiveState>, NiceMock<MockStoreActiveState>, NiceMock<MockBind>, NiceMock<MockWatchDescend>,
    NiceMock<MockGetNodeGoals>, NiceMock<MockGetCallSite>, NiceMock<MockMakeChildQueries>>;

struct PudChildActivatorTest : public ::testing::Test {
    NiceMock<MockGetActiveState> get_states;
    NiceMock<MockStoreActiveState> store_states;
    NiceMock<MockBind> binder;
    NiceMock<MockWatchDescend> watcher;
    NiceMock<MockGetNodeGoals> goals;
    NiceMock<MockGetCallSite> call_sites;
    NiceMock<MockMakeChildQueries> query_maker;
    test_child_activator_t activator{get_states, store_states, binder, watcher, goals, call_sites, query_maker};
    pud_goal_list node_goals{.goals = {}, .next_id = 0};
    pud_structure_watch parent_watch;
    pud_structure_watch child_watch;
    pud_ubs_state child_bindings;
    pud_active_state parent_state;

    void fork_child_query_to_sixty(pud_goal_queries& made, pud_node_id, const pud_ubs_state&, pud_body_goal_id) {
        made.heads = queries_t{}.set(1, 60);
    }

    void SetUp() override {
        parent_watch.goal_to_reps = parent_watch.goal_to_reps.set(1, {});
        child_watch.goal_to_reps = child_watch.goal_to_reps.set(2, {});
        child_bindings.parents = child_bindings.parents.set(5, 1);
        parent_state = pud_active_state{.bindings = {}, .watch = parent_watch, .queries = queries_t{}.set(1, 41)};
        ON_CALL(goals, get(_)).WillByDefault(ReturnRef(node_goals));
        ON_CALL(get_states, get(3)).WillByDefault(ReturnRef(parent_state));
        ON_CALL(call_sites, get(3)).WillByDefault(Return(0));
        ON_CALL(binder, bind(_, 4)).WillByDefault(Return(child_bindings));
        ON_CALL(watcher, descend(_, _, _, _, _))
            .WillByDefault(Return(pud_structure_watch_change{.watch = child_watch, .changed_goals = {1}}));
    }
};

}

TEST_F(PudChildActivatorTest, AChildContinuesItsParentsBindingsWatchAndQueries) {
    EXPECT_CALL(watcher, descend(parent_watch, child_bindings, 4u, 0u, _));
    EXPECT_CALL(query_maker, make_child_queries(parent_state.queries, 0u))
        .WillOnce(Return(pud_goal_queries{.heads = queries_t{}.set(1, 41), .refuted = false}));
    EXPECT_CALL(query_maker, fork_changed_goal(_, 4u, child_bindings, 1u))
        .WillOnce(Invoke(this, &PudChildActivatorTest::fork_child_query_to_sixty));
    EXPECT_CALL(query_maker, admit_child_queries(_, 4u, child_bindings, parent_state.queries));
    EXPECT_CALL(store_states, store(4, pud_active_state{.bindings = child_bindings, .watch = child_watch, .queries = queries_t{}.set(1, 60)}));
    EXPECT_FALSE(activator.activate_child(3, 4));
}

TEST_F(PudChildActivatorTest, AGoalWithoutCandidatesIsStoredAndReportedRefuted) {
    ON_CALL(query_maker, make_child_queries(_, _)).WillByDefault(Return(pud_goal_queries{.heads = {}, .refuted = true}));
    EXPECT_CALL(store_states, store(4, _));
    EXPECT_TRUE(activator.activate_child(3, 4));
}
