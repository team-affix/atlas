#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_trail_child_activator.hpp"
#include "pud_test_node.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "value_objects/pud_goal_trails.hpp"
#include "value_objects/pud_inference.hpp"
#include "value_objects/pud_node_trails.hpp"
#include "value_objects/pud_structure_watch.hpp"
#include "value_objects/pud_structure_watch_change.hpp"
#include "value_objects/pud_ubs_state.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SaveArg;

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
struct MockGetNodeTrails {
    MOCK_METHOD(const pud_node_trails&, get, (pud_global_node_id));
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
    MOCK_METHOD(
        pud_structure_watch_change,
        descend,
        (const pud_structure_watch&, const pud_ubs_state&, pud_global_node_id, const pud_goal_list&));
};

using test_activator_t = pud_trail_child_activator<
    NiceMock<MockGetNodeLvc>, NiceMock<MockGetNodeBindings>, NiceMock<MockGetNodeWatch>, NiceMock<MockGetNodeTrails>,
    NiceMock<MockStoreNodeLvc>, NiceMock<MockStoreNodeBindings>, NiceMock<MockStoreNodeWatch>,
    NiceMock<MockStoreNodeTrails>, NiceMock<MockGetInference>, NiceMock<MockGetNodeGoals>, NiceMock<MockGetNodeAxiom>,
    NiceMock<MockBuildChildGoalList>, NiceMock<MockStoreNodeGoals>, NiceMock<MockStoreNodeAxiom>,
    NiceMock<MockIncrementLiveChildCount>, NiceMock<MockBind>, NiceMock<MockWatchDescend>>;

pud_goal_trails trails_on(pud_node_id candidate, bool forked) {
    const pud_candidate_trail trail{
        .candidate = pud_test_node(candidate), .witness_leaves = {pud_test_node(candidate)}};
    return pud_goal_trails{.candidates = {trail}, .forked = forked};
}

// The parent 1 holds trails for goals 0, 2 and 3; the child 2 closes goal 3 and its bindings change goal 2's structure.
struct PudTrailChildActivatorTest : public ::testing::Test {
    NiceMock<MockGetNodeLvc> get_lvc;
    NiceMock<MockGetNodeBindings> get_bindings;
    NiceMock<MockGetNodeWatch> get_watch;
    NiceMock<MockGetNodeTrails> get_trails;
    NiceMock<MockStoreNodeLvc> store_lvc;
    NiceMock<MockStoreNodeBindings> store_bindings;
    NiceMock<MockStoreNodeWatch> store_watch;
    NiceMock<MockStoreNodeTrails> store_trails;
    NiceMock<MockGetInference> inferences;
    NiceMock<MockGetNodeGoals> get_goals;
    NiceMock<MockGetNodeAxiom> get_axiom;
    NiceMock<MockBuildChildGoalList> goal_lists;
    NiceMock<MockStoreNodeGoals> store_goals;
    NiceMock<MockStoreNodeAxiom> store_axiom;
    NiceMock<MockIncrementLiveChildCount> live_children;
    NiceMock<MockBind> binder;
    NiceMock<MockWatchDescend> watcher;
    test_activator_t activator{
        get_lvc, get_bindings, get_watch, get_trails, store_lvc, store_bindings, store_watch, store_trails,
        inferences, get_goals, get_axiom, goal_lists, store_goals, store_axiom, live_children, binder, watcher};
    expr added_goal{.content = expr::var{.index = 5}};
    pud_inference inference{
        .parent = 1, .call_site = 3, .substitutions = {}, .body_goals = {&added_goal}, .var_count = 2};
    pud_goal_list parent_goals{.goals = {}, .next_id = 4};
    pud_goal_list child_goals{.goals = immer::map<pud_body_goal_id, const expr*>{}.set(4, &added_goal), .next_id = 5};
    pud_ubs_state parent_bindings{};
    pud_ubs_state child_bindings{};
    pud_structure_watch parent_watch{.rep_to_goals = {}};
    pud_structure_watch child_watch{.rep_to_goals = immer::map<uint32_t, immer::set<pud_body_goal_id>>{}.set(1, {})};
    pud_node_trails parent_trails =
        pud_node_trails{}.set(0, trails_on(10, false)).set(2, trails_on(12, false)).set(3, trails_on(13, false));
    pud_node_trails stored{};

    void SetUp() override {
        ON_CALL(inferences, get_inference(pud_test_node(2))).WillByDefault(ReturnRef(inference));
        ON_CALL(get_goals, get(pud_test_node(1))).WillByDefault(ReturnRef(parent_goals));
        ON_CALL(get_bindings, get(pud_test_node(1))).WillByDefault(ReturnRef(parent_bindings));
        ON_CALL(get_watch, get(pud_test_node(1))).WillByDefault(ReturnRef(parent_watch));
        ON_CALL(get_trails, get(pud_test_node(1))).WillByDefault(ReturnRef(parent_trails));
        ON_CALL(get_lvc, get(pud_test_node(1))).WillByDefault(Return(6u));
        ON_CALL(get_axiom, get(pud_test_node(1))).WillByDefault(Return(pud_test_node(0)));
        ON_CALL(goal_lists, build(_, _, _)).WillByDefault(Return(child_goals));
        ON_CALL(binder, bind(_, _)).WillByDefault(Return(child_bindings));
        ON_CALL(watcher, descend(_, _, _, _))
            .WillByDefault(Return(pud_structure_watch_change{.watch = child_watch, .changed_goals = {2}}));
        ON_CALL(store_trails, store(pud_test_node(2), _)).WillByDefault(SaveArg<1>(&stored));
    }
};

}

TEST_F(PudTrailChildActivatorTest, TheChildStoresWhatEveryLiveLeafHas) {
    EXPECT_CALL(goal_lists, build(parent_goals, 3u, inference.body_goals)).WillOnce(Return(child_goals));
    EXPECT_CALL(store_goals, store(pud_test_node(2), child_goals));
    EXPECT_CALL(store_axiom, store(pud_test_node(2), pud_test_node(0)));
    EXPECT_CALL(live_children, increment(pud_test_node(1)));
    EXPECT_CALL(store_lvc, store(pud_test_node(2), 8u));
    EXPECT_CALL(store_bindings, store(pud_test_node(2), child_bindings));
    activator.activate_child(pud_test_node(1), pud_test_node(2));
}

TEST_F(PudTrailChildActivatorTest, TheChildsWatchDescendsFromItsParentsUnderItsBindings) {
    EXPECT_CALL(watcher, descend(parent_watch, child_bindings, pud_test_node(2), child_goals))
        .WillOnce(Return(pud_structure_watch_change{.watch = child_watch, .changed_goals = {}}));
    EXPECT_CALL(store_watch, store(pud_test_node(2), child_watch));
    activator.activate_child(pud_test_node(1), pud_test_node(2));
}

TEST_F(PudTrailChildActivatorTest, TheClosedGoalsTrailsAreDropped) {
    activator.activate_child(pud_test_node(1), pud_test_node(2));
    EXPECT_EQ(stored.find(3), nullptr);
}

TEST_F(PudTrailChildActivatorTest, AGoalWhoseStructureChangedHasItsTrailsForked) {
    activator.activate_child(pud_test_node(1), pud_test_node(2));
    ASSERT_NE(stored.find(2), nullptr);
    EXPECT_EQ(stored.find(2)->get(), trails_on(12, true));
}

TEST_F(PudTrailChildActivatorTest, AnUnchangedGoalSharesItsParentsTrails) {
    activator.activate_child(pud_test_node(1), pud_test_node(2));
    ASSERT_NE(stored.find(0), nullptr);
    EXPECT_EQ(&stored.find(0)->get(), &parent_trails.find(0)->get());
}

TEST_F(PudTrailChildActivatorTest, TheInferencesNewGoalsHaveNoTrails) {
    activator.activate_child(pud_test_node(1), pud_test_node(2));
    EXPECT_EQ(stored.find(4), nullptr);
}

TEST_F(PudTrailChildActivatorTest, AChildIsNeverRefutedOnArrival) {
    EXPECT_FALSE(activator.activate_child(pud_test_node(1), pud_test_node(2)));
}
