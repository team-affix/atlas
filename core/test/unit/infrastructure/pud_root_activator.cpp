#include <optional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_root_activator.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

using queries_t = immer::map<pud_body_goal_id, pud_mhq_head_id>;

struct MockStoreActiveState {
    MOCK_METHOD(void, store, (pud_global_node_id, pud_active_state));
};
struct MockWatchRoot {
    MOCK_METHOD(pud_structure_watch, watch_root, (const pud_ubs_state&, const pud_goal_list&));
};
struct MockGetNodeGoals {
    MOCK_METHOD(const pud_goal_list&, get, (pud_global_node_id));
};
struct MockOpenQuery {
    MOCK_METHOD(std::optional<pud_mhq_head_id>, open, (pud_global_node_id, const pud_ubs_state&, pud_body_goal_id));
};
struct MockHoldQuery {
    MOCK_METHOD(void, hold, (pud_mhq_head_id, pud_global_node_id, pud_body_goal_id));
};

using test_root_activator_t = pud_root_activator<
    NiceMock<MockStoreActiveState>, NiceMock<MockWatchRoot>, NiceMock<MockGetNodeGoals>, NiceMock<MockOpenQuery>,
    NiceMock<MockHoldQuery>>;

struct PudRootActivatorTest : public ::testing::Test {
    NiceMock<MockStoreActiveState> store_states;
    NiceMock<MockWatchRoot> watcher;
    NiceMock<MockGetNodeGoals> goals;
    NiceMock<MockOpenQuery> opener;
    NiceMock<MockHoldQuery> holders;
    test_root_activator_t activator{store_states, watcher, goals, opener, holders};
    expr goal_a{expr::functor{1, {}}};
    expr goal_b{expr::functor{2, {}}};
    pud_goal_list root_goals = pud_goal_list{.goals = {}, .next_id = 0}.adding(&goal_a).adding(&goal_b);
    pud_structure_watch root_watch;

    void SetUp() override {
        root_watch.goal_to_reps = root_watch.goal_to_reps.set(0, {});
        ON_CALL(goals, get(_)).WillByDefault(ReturnRef(root_goals));
        ON_CALL(watcher, watch_root(_, _)).WillByDefault(Return(root_watch));
        ON_CALL(opener, open(pud_test_node(2), pud_ubs_state{}, 0)).WillByDefault(Return(50));
        ON_CALL(opener, open(pud_test_node(2), pud_ubs_state{}, 1)).WillByDefault(Return(51));
    }
};

}

TEST_F(PudRootActivatorTest, ARootOpensAndHoldsAQueryPerGoalFromEmptyBindingsAndStoresItsState) {
    EXPECT_CALL(holders, hold(50u, pud_test_node(2), 0u));
    EXPECT_CALL(holders, hold(51u, pud_test_node(2), 1u));
    EXPECT_CALL(store_states, store(pud_test_node(2), pud_active_state{.bindings = {}, .watch = root_watch, .queries = queries_t{}.set(0, 50).set(1, 51)}));
    EXPECT_FALSE(activator.activate_root(pud_test_node(2)));
}

TEST_F(PudRootActivatorTest, AGoalWithoutCandidatesRefutesTheRootButTheOtherQueriesAreStillHeld) {
    ON_CALL(opener, open(pud_test_node(2), _, 0)).WillByDefault(Return(std::nullopt));
    EXPECT_CALL(holders, hold(51u, pud_test_node(2), 1u));
    EXPECT_CALL(store_states, store(pud_test_node(2), pud_active_state{.bindings = {}, .watch = root_watch, .queries = queries_t{}.set(1, 51)}));
    EXPECT_TRUE(activator.activate_root(pud_test_node(2)));
}
