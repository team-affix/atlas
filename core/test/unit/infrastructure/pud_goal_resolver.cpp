#include <gtest/gtest.h>
#include "infrastructure/pud_goal_resolver.hpp"

namespace {

using goals_t = immer::map<pud_body_goal_id, const expr*>;

struct PudGoalResolverTest : public ::testing::Test {
    pud_goal_resolver resolver;
    expr a{expr::functor{1, {}}};
    expr b{expr::functor{2, {}}};
    expr c{expr::functor{3, {}}};
};

}

TEST_F(PudGoalResolverTest, ResolveGoalNumbersAddedGoalsFromTheParentsNextId) {
    const pud_goal_list parent{.goals = {}, .next_id = 4};
    const pud_goal_list list = resolver.resolve_goal(parent, 99, {&a, &b});
    EXPECT_EQ(list.goals, goals_t{}.set(4, &a).set(5, &b));
    EXPECT_EQ(list.next_id, 6u);
}

TEST_F(PudGoalResolverTest, ResolveGoalWithNoAddedGoalsOnlyClosesTheCallSite) {
    const pud_goal_list parent{
        .goals = goals_t{}.set(0, &a).set(1, &b).set(2, &c), .next_id = 3};
    const pud_goal_list list = resolver.resolve_goal(parent, 1, {});
    EXPECT_EQ(list.goals, goals_t{}.set(0, &a).set(2, &c));
    EXPECT_EQ(list.next_id, 3u);
}

TEST_F(PudGoalResolverTest, ResolveGoalClosingRemovesOnlyThatGoalAndKeepsTheNextId) {
    const pud_goal_list parent{
        .goals = goals_t{}.set(0, &a).set(1, &b).set(2, &c), .next_id = 3};
    const pud_goal_list list = resolver.resolve_goal(parent, 1, {});
    EXPECT_EQ(list.goals, goals_t{}.set(0, &a).set(2, &c));
    EXPECT_EQ(list.next_id, 3u);
}

TEST_F(PudGoalResolverTest, ResolveGoalClosingAnAbsentIdIsANoOp) {
    const pud_goal_list parent{.goals = goals_t{}.set(0, &a).set(1, &b), .next_id = 2};
    const pud_goal_list list = resolver.resolve_goal(parent, 99, {&c});
    EXPECT_EQ(list.goals, goals_t{}.set(0, &a).set(1, &b).set(2, &c));
    EXPECT_EQ(list.next_id, 3u);
}

TEST_F(PudGoalResolverTest, AChildKeepsItsParentsGoalsUnderTheSameIdsAndNeverReusesAClosedId) {
    const pud_goal_list parent{.goals = goals_t{}.set(0, &a).set(1, &b), .next_id = 2};
    const pud_goal_list child = resolver.resolve_goal(parent, 0, {&c});
    EXPECT_EQ(child.goals, goals_t{}.set(1, &b).set(2, &c));
    EXPECT_EQ(child.next_id, 3u);
}

TEST_F(PudGoalResolverTest, ResolveGoalLeavesTheOriginalListAlone) {
    const pud_goal_list parent{.goals = goals_t{}.set(0, &a).set(1, &b), .next_id = 2};
    resolver.resolve_goal(parent, 0, {});
    EXPECT_EQ(parent.goals, goals_t{}.set(0, &a).set(1, &b));
    EXPECT_EQ(parent.next_id, 2u);
}
