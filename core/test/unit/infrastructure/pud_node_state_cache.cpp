#include <optional>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_node_state_cache.hpp"
#include "pud_test_node.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "value_objects/pud_inference.hpp"
#include "value_objects/pud_node_state.hpp"
#include "value_objects/pud_ubs_state.hpp"
#include "value_objects/rule.hpp"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetParent {
    MOCK_METHOD(std::optional<pud_global_node_id>, get_parent, (pud_global_node_id), (const));
};
struct MockGetAxiom {
    MOCK_METHOD(const rule&, get_axiom, (pud_global_node_id), (const));
};
struct MockGetInference {
    MOCK_METHOD(const pud_inference&, get_inference, (pud_global_node_id), (const));
};
struct MockBuildRootGoalList {
    MOCK_METHOD(pud_goal_list, build, ((const std::vector<const expr*>&)), (const));
};
struct MockBuildChildGoalList {
    MOCK_METHOD(
        pud_goal_list,
        build,
        (const pud_goal_list&, pud_body_goal_id, (const std::vector<const expr*>&)),
        (const));
};
struct MockBind {
    MOCK_METHOD(pud_ubs_state, bind, (const pud_ubs_state&, pud_global_node_id), (const));
};

// Each built goal list records how many goals it was built from in next_id, so a state's goals tell its path length.
pud_goal_list pud_root_goals(const std::vector<const expr*>& body) {
    return pud_goal_list{.goals = {}, .next_id = body.size()};
}
pud_goal_list pud_child_goals(const pud_goal_list& parent, pud_body_goal_id, const std::vector<const expr*>&) {
    return pud_goal_list{.goals = {}, .next_id = parent.next_id + 1};
}
// Binding a child records it as a class root, so a state's bindings tell the nodes folded into it.
pud_ubs_state pud_bind_child(const pud_ubs_state& parent, pud_global_node_id child) {
    return pud_ubs_state{.parents = parent.parents.set(static_cast<uint32_t>(child.node), 0), .classes = {}};
}

using test_cache_t = pud_node_state_cache<
    NiceMock<MockGetParent>, NiceMock<MockGetAxiom>, NiceMock<MockGetInference>, NiceMock<MockBuildRootGoalList>,
    NiceMock<MockBuildChildGoalList>, NiceMock<MockBind>>;

// The stored tree is 0 -> 1 -> 2, and 0 -> 3. Axiom 0 has two body goals and three variables; each inference adds two.
struct PudNodeStateCacheTest : public ::testing::Test {
    NiceMock<MockGetParent> parents;
    NiceMock<MockGetAxiom> axioms;
    NiceMock<MockGetInference> inferences;
    NiceMock<MockBuildRootGoalList> root_goals;
    NiceMock<MockBuildChildGoalList> child_goals;
    NiceMock<MockBind> binder;
    expr goal{.content = expr::var{.index = 0}};
    rule axiom{&goal, {&goal, &goal}, 3};
    pud_inference inference{.parent = 0, .call_site = 0, .substitutions = {}, .body_goals = {&goal}, .var_count = 2};

    test_cache_t make_cache(size_t budget) {
        return test_cache_t{parents, axioms, inferences, root_goals, child_goals, binder, budget};
    }

    void SetUp() override {
        ON_CALL(parents, get_parent(_)).WillByDefault(Return(std::nullopt));
        ON_CALL(parents, get_parent(pud_test_node(1))).WillByDefault(Return(pud_test_node(0)));
        ON_CALL(parents, get_parent(pud_test_node(2))).WillByDefault(Return(pud_test_node(1)));
        ON_CALL(parents, get_parent(pud_test_node(3))).WillByDefault(Return(pud_test_node(0)));
        ON_CALL(axioms, get_axiom(pud_test_node(0))).WillByDefault(ReturnRef(axiom));
        ON_CALL(inferences, get_inference(_)).WillByDefault(ReturnRef(inference));
        ON_CALL(root_goals, build(_)).WillByDefault(Invoke(&pud_root_goals));
        ON_CALL(child_goals, build(_, _, _)).WillByDefault(Invoke(&pud_child_goals));
        ON_CALL(binder, bind(_, _)).WillByDefault(Invoke(&pud_bind_child));
    }
};

}

TEST_F(PudNodeStateCacheTest, ARootsStateIsItsAxiomsGoalsAndVariablesUnbound) {
    test_cache_t cache = make_cache(1 << 20);
    const pud_node_state& state = cache.get(pud_test_node(0));
    EXPECT_EQ(state.goals.next_id, 2u);
    EXPECT_EQ(state.axiom, pud_test_node(0));
    EXPECT_EQ(state.depth, 0u);
    EXPECT_EQ(state.lvc, 3u);
    EXPECT_EQ(state.bindings, pud_ubs_state{});
}

TEST_F(PudNodeStateCacheTest, ANodesStateFoldsEachInferenceOnItsPath) {
    test_cache_t cache = make_cache(1 << 20);
    const pud_node_state& state = cache.get(pud_test_node(2));
    EXPECT_EQ(state.goals.next_id, 4u);
    EXPECT_EQ(state.axiom, pud_test_node(0));
    EXPECT_EQ(state.depth, 2u);
    EXPECT_EQ(state.lvc, 7u);
    EXPECT_EQ(state.bindings.parents.size(), 2u);
    EXPECT_NE(state.bindings.parents.find(1), nullptr);
    EXPECT_NE(state.bindings.parents.find(2), nullptr);
}

TEST_F(PudNodeStateCacheTest, AKeptStateIsReadAgainWithoutFolding) {
    test_cache_t cache = make_cache(1 << 20);
    cache.get(pud_test_node(2));
    EXPECT_CALL(binder, bind(_, _)).Times(0);
    cache.get(pud_test_node(2));
    EXPECT_EQ(cache.hits(), 1u);
    EXPECT_EQ(cache.misses(), 1u);
}

TEST_F(PudNodeStateCacheTest, AMissFoldsFromTheNearestKeptAncestor) {
    test_cache_t cache = make_cache(1 << 20);
    cache.get(pud_test_node(1));
    EXPECT_CALL(binder, bind(_, _)).Times(AnyNumber());
    EXPECT_CALL(binder, bind(_, pud_test_node(1))).Times(0);
    EXPECT_CALL(root_goals, build(_)).Times(0);
    cache.get(pud_test_node(2));
}

TEST_F(PudNodeStateCacheTest, TrimmingDropsTheLeastRecentlyReadStatesUntilTheBudgetHolds) {
    test_cache_t cache = make_cache(0);
    cache.get(pud_test_node(3));
    cache.trim();
    EXPECT_CALL(root_goals, build(_)).Times(1);
    cache.get(pud_test_node(0));
}

TEST_F(PudNodeStateCacheTest, StatesAreKeptUntilTrimmedEvenOverBudget) {
    test_cache_t cache = make_cache(0);
    const pud_node_state& state = cache.get(pud_test_node(2));
    cache.get(pud_test_node(3));
    EXPECT_EQ(state.depth, 2u);
    EXPECT_CALL(root_goals, build(_)).Times(0);
    cache.get(pud_test_node(0));
}

TEST_F(PudNodeStateCacheTest, AStateWorkedOutAgainEqualsTheOneDropped) {
    test_cache_t cache = make_cache(0);
    const pud_node_state first = cache.get(pud_test_node(2));
    cache.trim();
    EXPECT_EQ(cache.get(pud_test_node(2)), first);
}

TEST_F(PudNodeStateCacheTest, AnEvictedStateIsWorkedOutAgainWhenRead) {
    test_cache_t cache = make_cache(1 << 20);
    cache.get(pud_test_node(2));
    cache.evict(pud_test_node(2));
    EXPECT_CALL(binder, bind(_, pud_test_node(2)));
    cache.get(pud_test_node(2));
}
