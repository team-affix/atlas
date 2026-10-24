#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "reference_pud_goal_list.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_module.hpp"

using ::testing::IsEmpty;
using ::testing::Pair;
using ::testing::UnorderedElementsAre;

// The goal-list rebuild the reload tests pick unfoldable goals with, checked on a hand-built module.
// Axiom 0 is p :- a, b, c.
struct ReferencePudGoalListTest : public ::testing::Test {
    test_functors functors;
    expr_pool exprs;
    const expr* p = exprs.make_functor(functors.id("p"), {});
    const expr* a = exprs.make_functor(functors.id("a"), {});
    const expr* b = exprs.make_functor(functors.id("b"), {});
    const expr* c = exprs.make_functor(functors.id("c"), {});
    const expr* d = exprs.make_functor(functors.id("d"), {});
    const expr* e = exprs.make_functor(functors.id("e"), {});
    pud_module module{7, {}, {rule(p, {a, b, c}, 0), rule(a, {}, 0)}};
};

TEST_F(ReferencePudGoalListTest, AnAxiomsGoalsAreItsBodyInOrder) {
    const pud_goal_list goals = reference_pud_goal_list(module, 0);
    EXPECT_THAT(goals.goals, UnorderedElementsAre(Pair(0u, a), Pair(1u, b), Pair(2u, c)));
    EXPECT_EQ(goals.next_id, 3u);
}

TEST_F(ReferencePudGoalListTest, AFactHasNoGoals) {
    EXPECT_THAT(reference_pud_goal_list(module, 1).goals, IsEmpty());
}

TEST_F(ReferencePudGoalListTest, AChildClosesItsCallSiteAndAddsItsBodyGoals) {
    const pud_node_id child = module.store_inference(
        pud_inference{.parent = 0, .call_site = 1, .substitutions = {}, .body_goals = {d, e}, .var_count = 0});
    const pud_goal_list goals = reference_pud_goal_list(module, child);
    EXPECT_THAT(goals.goals, UnorderedElementsAre(Pair(0u, a), Pair(2u, c), Pair(3u, d), Pair(4u, e)));
    EXPECT_EQ(goals.next_id, 5u);
}

TEST_F(ReferencePudGoalListTest, AGrandchildBuildsOnItsParentsGoals) {
    const pud_node_id child = module.store_inference(
        pud_inference{.parent = 0, .call_site = 1, .substitutions = {}, .body_goals = {d}, .var_count = 0});
    const pud_node_id grandchild = module.store_inference(
        pud_inference{.parent = child, .call_site = 3, .substitutions = {}, .body_goals = {}, .var_count = 0});
    const pud_goal_list goals = reference_pud_goal_list(module, grandchild);
    EXPECT_THAT(goals.goals, UnorderedElementsAre(Pair(0u, a), Pair(2u, c)));
    EXPECT_EQ(goals.next_id, 4u);
}
