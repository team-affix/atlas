#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_root_activator.hpp"
#include "value_objects/pud_goal_list.hpp"

using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

using queries_t = immer::map<pud_body_goal_id, pud_mhq_head_id>;

struct MockStoreActiveState {
    MOCK_METHOD(void, store, (pud_node_id, pud_active_state));
};
struct MockWatchRoot {
    MOCK_METHOD(pud_structure_watch, watch_root, (const pud_ubs_state&, const pud_goal_list&));
};
struct MockGetNodeGoals {
    MOCK_METHOD(const pud_goal_list&, get, (pud_node_id));
};
struct MockOpenQueries {
    MOCK_METHOD(void, open_queries, (pud_goal_queries&, pud_node_id, const pud_ubs_state&, const queries_t&));
};

using test_root_activator_t =
    pud_root_activator<NiceMock<MockStoreActiveState>, NiceMock<MockWatchRoot>, NiceMock<MockGetNodeGoals>, NiceMock<MockOpenQueries>>;

struct PudRootActivatorTest : public ::testing::Test {
    NiceMock<MockStoreActiveState> store_states;
    NiceMock<MockWatchRoot> watcher;
    NiceMock<MockGetNodeGoals> goals;
    NiceMock<MockOpenQueries> query_maker;
    test_root_activator_t activator{store_states, watcher, goals, query_maker};
    pud_goal_list node_goals{.goals = {}, .next_id = 0};
    pud_structure_watch root_watch;

    void open_query_fifty(pud_goal_queries& made, pud_node_id, const pud_ubs_state&, const queries_t&) {
        made.heads = queries_t{}.set(0, 50);
    }

    void open_nothing_refuted(pud_goal_queries& made, pud_node_id, const pud_ubs_state&, const queries_t&) {
        made.refuted = true;
    }

    void SetUp() override {
        root_watch.goal_to_reps = root_watch.goal_to_reps.set(0, {});
        ON_CALL(goals, get(_)).WillByDefault(ReturnRef(node_goals));
        ON_CALL(watcher, watch_root(_, _)).WillByDefault(Return(root_watch));
    }
};

}

TEST_F(PudRootActivatorTest, ARootStartsFromEmptyBindingsWithNothingInheritedAndStoresItsState) {
    EXPECT_CALL(query_maker, open_queries(_, 2u, pud_ubs_state{}, queries_t{}))
        .WillOnce(Invoke(this, &PudRootActivatorTest::open_query_fifty));
    EXPECT_CALL(store_states, store(2, pud_active_state{.bindings = {}, .watch = root_watch, .queries = queries_t{}.set(0, 50)}));
    EXPECT_FALSE(activator.activate_root(2));
}

TEST_F(PudRootActivatorTest, ARootWithAGoalWithoutCandidatesIsStoredAndReportedRefuted) {
    ON_CALL(query_maker, open_queries(_, _, _, _)).WillByDefault(Invoke(this, &PudRootActivatorTest::open_nothing_refuted));
    EXPECT_CALL(store_states, store(2, _));
    EXPECT_TRUE(activator.activate_root(2));
}
