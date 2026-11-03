#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_focus_extender.hpp"
#include "infrastructure/pud_focus_seeder.hpp"
#include "infrastructure/pud_focus_set.hpp"
#include "infrastructure/pud_frontier.hpp"
#include "infrastructure/pud_search_driver.hpp"
#include "infrastructure/pud_searcher.hpp"
#include "infrastructure/pud_solved_leaf_finder.hpp"
#include "infrastructure/pud_pending_conflict_store.hpp"
#include "infrastructure/pud_solution_store.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "value_objects/pud_leaf_goal_query.hpp"

using ::testing::Return;

namespace {

pud_global_node_id node(pud_node_id id) {
    return pud_global_node_id{.module = 7, .node = id};
}

struct MockGetChildren {
    MOCK_METHOD(const std::vector<pud_node_id>&, get_children, (pud_global_node_id));
};

struct MockIsNodeRefuted {
    MOCK_METHOD(bool, is_refuted, (pud_global_node_id));
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
    MOCK_METHOD(pud_unfold_outcome, unfold_one, ());
};

struct MockListQueries {
    MOCK_METHOD(std::vector<pud_leaf_goal_query>, list, (), (const));
};

struct PudSearchTickIntegrationTest : public ::testing::Test {
    pud_focus_set focus;
    MockGetChildren children;
    pud_focus_seeder<MockGetChildren, pud_focus_set, pud_focus_set> seeder{
        node(0), children, focus, focus};
    pud_focus_extender<pud_focus_set, pud_focus_set> extender{focus, focus};
    pud_frontier frontier;
    MockIsNodeRefuted refuted;
    MockGetNodeGoals goals;
    pud_solved_leaf_finder<
        pud_frontier, pud_focus_set, MockIsNodeRefuted, MockGetNodeGoals>
        finder{frontier, focus, refuted, goals};
    pud_solution_store solutions;
    pud_pending_conflict_store conflicts;
    MockTakeSolution take;
    MockRefuteBlocked refute;
    MockUnfoldOne unfold;
    MockListQueries queries;
    pud_searcher<
        pud_solution_store, pud_pending_conflict_store, MockTakeSolution, MockRefuteBlocked,
        MockUnfoldOne, MockListQueries>
        searcher{solutions, conflicts, take, refute, unfold, queries, 1};
    pud_search_driver search{searcher.search()};
    pud_goal_list empty;
    std::vector<pud_node_id> axiom_children{1};
    std::vector<pud_node_id> none;
};

}

TEST_F(PudSearchTickIntegrationTest, SeedingThenExtendingMarksTheConjectureTree) {
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

TEST_F(PudSearchTickIntegrationTest, TheFinderReturnsTheSmallestSolvedFocusLeaf) {
    focus.insert(node(0));
    focus.insert(node(2));
    frontier.apply_register(pud_frontier_change{.offspring = {node(2), node(0), node(1)}, .refuted = {}});
    EXPECT_CALL(refuted, is_refuted).WillRepeatedly(Return(false));
    EXPECT_CALL(goals, get).WillRepeatedly(::testing::ReturnRef(empty));
    const std::optional<pud_global_node_id> found = finder.find();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found.value(), node(0));
}

TEST_F(PudSearchTickIntegrationTest, ErasingAReturnedLeafMakesTheNextSolvedLeafVisible) {
    focus.insert(node(0));
    focus.insert(node(2));
    frontier.apply_register(pud_frontier_change{.offspring = {node(0), node(2)}, .refuted = {}});
    EXPECT_CALL(refuted, is_refuted).WillRepeatedly(Return(false));
    EXPECT_CALL(goals, get).WillRepeatedly(::testing::ReturnRef(empty));
    frontier.erase(finder.find().value());
    const std::optional<pud_global_node_id> next = finder.find();
    ASSERT_TRUE(next.has_value());
    EXPECT_EQ(next.value(), node(2));
}

TEST_F(PudSearchTickIntegrationTest, AConflictStepClearsThePreviousSolution) {
    const expr stored{expr::functor{4, {}}};
    solutions.set(&stored, 2);
    conflicts.add(1);
    ASSERT_TRUE(search.next());
    EXPECT_EQ(search.last_step(), pud_search_step::refuted);
    EXPECT_EQ(solutions.solution(), nullptr);
    EXPECT_EQ(solutions.solution_depth(), 0u);
    EXPECT_EQ(solutions.solution_count(), 1u);
    EXPECT_FALSE(conflicts.try_take());
}

TEST_F(PudSearchTickIntegrationTest, SpendingTheBudgetStopsTheSessionAndLeavesNoSolution) {
    const expr stored{expr::functor{4, {}}};
    solutions.set(&stored, 2);
    EXPECT_CALL(take, take()).WillOnce(Return(false)).WillOnce(Return(false));
    EXPECT_CALL(refute, refute_blocked()).WillOnce(Return(false)).WillOnce(Return(false));
    EXPECT_CALL(unfold, unfold_one()).WillOnce(Return(pud_unfold_outcome::off_focus));
    ASSERT_TRUE(search.next());
    EXPECT_EQ(search.last_step(), pud_search_step::unfolded);
    EXPECT_FALSE(search.next());
    EXPECT_EQ(searcher.stop_reason(), pud_search_stop_reason::budget_spent);
    EXPECT_EQ(solutions.solution(), nullptr);
    EXPECT_FALSE(search.next());
}
