#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_lodestar_focus_extender.hpp"
#include "infrastructure/pud_lodestar_focus_seeder.hpp"
#include "infrastructure/pud_lodestar_focus_set.hpp"
#include "infrastructure/pud_lodestar_frontier.hpp"
#include "infrastructure/pud_lodestar_search_stepper.hpp"
#include "infrastructure/pud_lodestar_solved_leaf_finder.hpp"
#include "infrastructure/pud_lodestar_unfold_budget_store.hpp"
#include "infrastructure/pud_node_depth_counter.hpp"
#include "infrastructure/pud_pending_conflict_store.hpp"
#include "infrastructure/pud_solution_store.hpp"
#include "value_objects/pud_goal_list.hpp"

using ::testing::Return;

namespace {

pud_global_node_id node(pud_node_id id) {
    return pud_global_node_id{.module = 7, .node = id};
}

struct MockGetChildren {
    MOCK_METHOD(const std::vector<pud_node_id>&, get_children, (pud_global_node_id));
};

struct MockGetParent {
    MOCK_METHOD(std::optional<pud_global_node_id>, get_parent, (pud_global_node_id));
};

struct MockCheckRefuted {
    MOCK_METHOD(bool, check_refuted, (pud_global_node_id));
};

struct MockGetNodeGoals {
    MOCK_METHOD(const pud_goal_list&, get, (pud_global_node_id));
};

struct MockTakeSolution {
    MOCK_METHOD(bool, take, ());
};

struct MockRefuteBlocked {
    MOCK_METHOD(bool, refute_blocked, ());
};

struct MockUnfoldOne {
    MOCK_METHOD(bool, unfold_one, ());
};

struct MockCheckLastUnfoldOnFocus {
    MOCK_METHOD(bool, last_unfold_on_focus, (), (const));
};

struct PudLodestarSearchTickIntegrationTest : public ::testing::Test {
    pud_lodestar_focus_set focus;
    MockGetChildren children;
    pud_lodestar_focus_seeder<MockGetChildren, pud_lodestar_focus_set, pud_lodestar_focus_set> seeder{
        node(0), children, focus, focus};
    pud_lodestar_focus_extender<pud_lodestar_focus_set, pud_lodestar_focus_set> extender{focus, focus};
    pud_lodestar_frontier frontier;
    MockCheckRefuted refuted;
    MockGetNodeGoals goals;
    pud_lodestar_solved_leaf_finder<
        pud_lodestar_frontier, pud_lodestar_focus_set, MockCheckRefuted, MockGetNodeGoals>
        finder{frontier, focus, refuted, goals};
    MockGetParent parents;
    pud_node_depth_counter<MockGetParent> depths{parents};
    pud_solution_store solutions;
    pud_pending_conflict_store conflicts;
    pud_lodestar_unfold_budget_store budget;
    MockTakeSolution take;
    MockRefuteBlocked refute;
    MockUnfoldOne unfold;
    MockCheckLastUnfoldOnFocus last_on_focus;
    pud_lodestar_search_stepper<
        pud_solution_store, pud_solution_store, pud_pending_conflict_store, pud_pending_conflict_store,
        pud_lodestar_unfold_budget_store, pud_lodestar_unfold_budget_store, pud_lodestar_unfold_budget_store,
        pud_lodestar_unfold_budget_store, MockTakeSolution, MockRefuteBlocked, MockUnfoldOne,
        MockCheckLastUnfoldOnFocus>
        stepper{
            solutions, solutions, conflicts, conflicts, budget, budget, budget, budget, take, refute, unfold,
            last_on_focus, 1};
    pud_goal_list empty;
    std::vector<pud_node_id> axiom_children{1};
    std::vector<pud_node_id> none;
};

}

TEST_F(PudLodestarSearchTickIntegrationTest, SeedingThenExtendingMarksTheConjectureTree) {
    focus.insert(node(0));
    EXPECT_CALL(children, get_children(node(0))).WillOnce(::testing::ReturnRef(axiom_children));
    EXPECT_CALL(children, get_children(node(1))).WillOnce(::testing::ReturnRef(none));
    seeder.seed(7);
    EXPECT_TRUE(focus.contains(node(1)));
    extender.extend(node(1), {node(2)});
    EXPECT_TRUE(focus.contains(node(2)));
    seeder.seed(3);
    EXPECT_FALSE(focus.contains(node(4)));
}

TEST_F(PudLodestarSearchTickIntegrationTest, TheFinderReturnsTheSmallestSolvedFocusLeaf) {
    focus.insert(node(0));
    focus.insert(node(2));
    frontier.apply_register(pud_frontier_change{.offspring = {node(2), node(0), node(1)}, .refuted = {}});
    EXPECT_CALL(refuted, check_refuted).WillRepeatedly(Return(false));
    EXPECT_CALL(goals, get).WillRepeatedly(::testing::ReturnRef(empty));
    const std::optional<pud_global_node_id> found = finder.find();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found.value(), node(0));
}

TEST_F(PudLodestarSearchTickIntegrationTest, ErasingAReturnedLeafMakesTheNextSolvedLeafVisible) {
    focus.insert(node(0));
    focus.insert(node(2));
    frontier.apply_register(pud_frontier_change{.offspring = {node(0), node(2)}, .refuted = {}});
    EXPECT_CALL(refuted, check_refuted).WillRepeatedly(Return(false));
    EXPECT_CALL(goals, get).WillRepeatedly(::testing::ReturnRef(empty));
    frontier.erase(finder.find().value());
    const std::optional<pud_global_node_id> next = finder.find();
    ASSERT_TRUE(next.has_value());
    EXPECT_EQ(next.value(), node(2));
}

TEST_F(PudLodestarSearchTickIntegrationTest, AConflictTickClearsThePreviousSolution) {
    const expr stored{expr::functor{4, {}}};
    solutions.set(&stored, 2);
    conflicts.set(1);
    EXPECT_TRUE(stepper.step());
    EXPECT_EQ(solutions.solution(), nullptr);
    EXPECT_EQ(solutions.solution_depth(), 0u);
    EXPECT_EQ(solutions.solution_count(), 1u);
    EXPECT_EQ(conflicts.get(), 0u);
}

TEST_F(PudLodestarSearchTickIntegrationTest, OneFocusUnfoldSpendsTheBudgetOnTheRealStore) {
    EXPECT_CALL(take, take()).WillOnce(Return(false)).WillOnce(Return(false));
    EXPECT_CALL(refute, refute_blocked()).WillOnce(Return(false)).WillOnce(Return(false));
    EXPECT_CALL(unfold, unfold_one()).WillOnce(Return(true));
    EXPECT_CALL(last_on_focus, last_unfold_on_focus()).WillOnce(Return(true));
    EXPECT_TRUE(stepper.step());
    EXPECT_EQ(budget.get_focus_unfolds(), 1u);
    EXPECT_TRUE(budget.get_stopped());
}

TEST_F(PudLodestarSearchTickIntegrationTest, DepthFollowsTheRealParentChain) {
    EXPECT_CALL(parents, get_parent(node(3))).WillOnce(Return(node(2)));
    EXPECT_CALL(parents, get_parent(node(2))).WillOnce(Return(node(0)));
    EXPECT_CALL(parents, get_parent(node(0))).WillOnce(Return(std::nullopt));
    EXPECT_EQ(depths.count(node(3)), 2u);
}
