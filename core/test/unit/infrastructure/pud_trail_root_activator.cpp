#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_trail_root_activator.hpp"
#include "pud_test_node.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "value_objects/pud_node_trails.hpp"
#include "value_objects/pud_structure_watch.hpp"
#include "value_objects/pud_ubs_state.hpp"
#include "value_objects/rule.hpp"

using ::testing::_;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SaveArg;

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
struct MockStoreNodeWatch {
    MOCK_METHOD(void, store, (pud_global_node_id, pud_structure_watch));
};
struct MockStoreNodeTrails {
    MOCK_METHOD(void, store, (pud_global_node_id, pud_node_trails));
};
struct MockWatchRoot {
    MOCK_METHOD(
        pud_structure_watch, watch_root, (const pud_ubs_state&, const pud_goal_list&, pud_module_id), (const));
};

using test_activator_t = pud_trail_root_activator<
    NiceMock<MockGetAxiom>, NiceMock<MockBuildRootGoalList>, NiceMock<MockStoreNodeGoals>,
    NiceMock<MockStoreNodeAxiom>, NiceMock<MockStoreNodeLvc>, NiceMock<MockStoreNodeBindings>,
    NiceMock<MockStoreNodeWatch>, NiceMock<MockStoreNodeTrails>, NiceMock<MockWatchRoot>>;

struct PudTrailRootActivatorTest : public ::testing::Test {
    NiceMock<MockGetAxiom> axioms;
    NiceMock<MockBuildRootGoalList> goal_lists;
    NiceMock<MockStoreNodeGoals> goals;
    NiceMock<MockStoreNodeAxiom> node_axioms;
    NiceMock<MockStoreNodeLvc> lvcs;
    NiceMock<MockStoreNodeBindings> bindings;
    NiceMock<MockStoreNodeWatch> watches;
    NiceMock<MockStoreNodeTrails> trails;
    NiceMock<MockWatchRoot> watcher;
    test_activator_t activator{axioms, goal_lists, goals, node_axioms, lvcs, bindings, watches, trails, watcher};
    expr body_goal{.content = expr::var{.index = 0}};
    expr head{.content = expr::var{.index = 1}};
    rule axiom{&head, {&body_goal}, 4};
    pud_goal_list built{.goals = immer::map<pud_body_goal_id, const expr*>{}.set(0, &body_goal), .next_id = 1};
    pud_structure_watch watch{.rep_to_goals = immer::map<uint32_t, immer::set<pud_body_goal_id>>{}.set(0, {})};

    void SetUp() override {
        ON_CALL(axioms, get_axiom(pud_test_node(2))).WillByDefault(ReturnRef(axiom));
        ON_CALL(goal_lists, build(_)).WillByDefault(Return(built));
        ON_CALL(watcher, watch_root(_, _, _)).WillByDefault(Return(watch));
    }
};

}

TEST_F(PudTrailRootActivatorTest, TheRootStoresWhatEveryLiveLeafHas) {
    EXPECT_CALL(goals, store(pud_test_node(2), built));
    EXPECT_CALL(node_axioms, store(pud_test_node(2), pud_test_node(2)));
    EXPECT_CALL(lvcs, store(pud_test_node(2), 4u));
    EXPECT_CALL(bindings, store(pud_test_node(2), pud_ubs_state{}));
    activator.activate_root(pud_test_node(2));
}

TEST_F(PudTrailRootActivatorTest, TheRootWatchesItsGoalsUnboundInItsModule) {
    EXPECT_CALL(watcher, watch_root(pud_ubs_state{}, built, k_test_module)).WillOnce(Return(watch));
    EXPECT_CALL(watches, store(pud_test_node(2), watch));
    activator.activate_root(pud_test_node(2));
}

TEST_F(PudTrailRootActivatorTest, TheRootHoldsNoTrails) {
    pud_node_trails stored = pud_node_trails{}.set(9, pud_goal_trails{.candidates = {}, .forked = true});
    EXPECT_CALL(trails, store(pud_test_node(2), _)).WillOnce(SaveArg<1>(&stored));
    activator.activate_root(pud_test_node(2));
    EXPECT_EQ(stored.size(), 0u);
}

TEST_F(PudTrailRootActivatorTest, ARootIsNeverRefutedOnArrival) {
    EXPECT_FALSE(activator.activate_root(pud_test_node(2)));
}
