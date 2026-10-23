#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_child_activator.hpp"
#include "value_objects/pud_goal_list.hpp"

using ::testing::_;
using ::testing::Eq;
using ::testing::InSequence;
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
struct MockForkQuery {
    MOCK_METHOD(void, fork_query, (pud_goal_queries&, pud_node_id, const pud_ubs_state&, pud_body_goal_id));
};
struct MockOpenQueries {
    MOCK_METHOD(void, open_queries, (pud_goal_queries&, pud_node_id, const pud_ubs_state&, const queries_t&));
};

using test_child_activator_t = pud_child_activator<
    NiceMock<MockGetActiveState>, NiceMock<MockStoreActiveState>, NiceMock<MockBind>, NiceMock<MockWatchDescend>,
    NiceMock<MockGetNodeGoals>, NiceMock<MockGetCallSite>, NiceMock<MockForkQuery>, NiceMock<MockOpenQueries>>;

struct PudChildActivatorTest : public ::testing::Test {
    NiceMock<MockGetActiveState> get_states;
    NiceMock<MockStoreActiveState> store_states;
    NiceMock<MockBind> binder;
    NiceMock<MockWatchDescend> watcher;
    NiceMock<MockGetNodeGoals> goals;
    NiceMock<MockGetCallSite> call_sites;
    NiceMock<MockForkQuery> forker;
    NiceMock<MockOpenQueries> opener;
    test_child_activator_t activator{get_states, store_states, binder, watcher, goals, call_sites, forker, opener};
    pud_goal_list node_goals{.goals = {}, .next_id = 0};
    pud_structure_watch parent_watch;
    pud_structure_watch child_watch;
    pud_ubs_state child_bindings;
    pud_active_state parent_state;

    void fork_to_sixty(pud_goal_queries& made, pud_node_id, const pud_ubs_state&, pud_body_goal_id goal) {
        made.heads = made.heads.set(goal, 60);
    }

    void refute_on_open(pud_goal_queries& made, pud_node_id, const pud_ubs_state&, const queries_t&) {
        made.refuted = true;
    }

    void SetUp() override {
        parent_watch.goal_to_reps = parent_watch.goal_to_reps.set(1, {});
        child_watch.goal_to_reps = child_watch.goal_to_reps.set(2, {});
        child_bindings.parents = child_bindings.parents.set(5, 1);
        parent_state = pud_active_state{.bindings = {}, .watch = parent_watch, .queries = queries_t{}.set(0, 40).set(1, 41)};
        ON_CALL(goals, get(_)).WillByDefault(ReturnRef(node_goals));
        ON_CALL(get_states, get(3)).WillByDefault(ReturnRef(parent_state));
        ON_CALL(call_sites, get(3)).WillByDefault(Return(0));
        ON_CALL(binder, bind(_, 4)).WillByDefault(Return(child_bindings));
        ON_CALL(watcher, descend(_, _, _, _, _))
            .WillByDefault(Return(pud_structure_watch_change{.watch = child_watch, .changed_goals = {1}}));
    }
};

}

TEST_F(PudChildActivatorTest, AChildInheritsAllButTheCallSitesQueryForksChangedGoalsThenOpensTheRest) {
    EXPECT_CALL(watcher, descend(parent_watch, child_bindings, 4u, 0u, _));
    InSequence in_order;
    const pud_goal_queries inherited{.heads = queries_t{}.set(1, 41), .refuted = false};
    const pud_goal_queries forked{.heads = queries_t{}.set(1, 60), .refuted = false};
    EXPECT_CALL(forker, fork_query(Eq(inherited), 4u, child_bindings, 1u))
        .WillOnce(Invoke(this, &PudChildActivatorTest::fork_to_sixty));
    EXPECT_CALL(opener, open_queries(Eq(forked), 4u, child_bindings, parent_state.queries));
    EXPECT_CALL(store_states, store(4, pud_active_state{.bindings = child_bindings, .watch = child_watch, .queries = queries_t{}.set(1, 60)}));
    EXPECT_FALSE(activator.activate_child(3, 4));
}

TEST_F(PudChildActivatorTest, AChildWithAGoalWithoutCandidatesIsStoredAndReportedRefuted) {
    ON_CALL(opener, open_queries(_, _, _, _)).WillByDefault(Invoke(this, &PudChildActivatorTest::refute_on_open));
    EXPECT_CALL(store_states, store(4, _));
    EXPECT_TRUE(activator.activate_child(3, 4));
}
