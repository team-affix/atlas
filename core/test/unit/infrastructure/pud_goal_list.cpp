#include <gtest/gtest.h>
#include "value_objects/pud_goal_list.hpp"

namespace {

using goals_t = immer::map<pud_body_goal_id, const expr*>;

struct PudGoalListTest : public ::testing::Test {
    expr a{expr::functor{1, {}}};
    expr b{expr::functor{2, {}}};
    expr c{expr::functor{3, {}}};
};

}

TEST_F(PudGoalListTest, AddingNumbersGoalsFromTheNextId) {
    const pud_goal_list list = pud_goal_list{.goals = {}, .next_id = 4}.adding(&a).adding(&b);
    EXPECT_EQ(list.goals, goals_t{}.set(4, &a).set(5, &b));
    EXPECT_EQ(list.next_id, 6u);
}

TEST_F(PudGoalListTest, ClosingRemovesOnlyThatGoalAndKeepsTheNextId) {
    const pud_goal_list list = pud_goal_list{.goals = {}, .next_id = 0}.adding(&a).adding(&b).adding(&c).closing(1);
    EXPECT_EQ(list.goals, goals_t{}.set(0, &a).set(2, &c));
    EXPECT_EQ(list.next_id, 3u);
}

TEST_F(PudGoalListTest, AChildKeepsItsParentsGoalsUnderTheSameIdsAndNeverReusesAClosedId) {
    const pud_goal_list parent = pud_goal_list{.goals = {}, .next_id = 0}.adding(&a).adding(&b);
    const pud_goal_list child = parent.closing(0).adding(&c);
    EXPECT_EQ(child.goals, goals_t{}.set(1, &b).set(2, &c));
}

TEST_F(PudGoalListTest, ClosingLeavesTheOriginalListAlone) {
    const pud_goal_list parent = pud_goal_list{.goals = {}, .next_id = 0}.adding(&a).adding(&b);
    parent.closing(0);
    EXPECT_EQ(parent.goals, goals_t{}.set(0, &a).set(1, &b));
}
