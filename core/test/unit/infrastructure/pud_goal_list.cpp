#include <vector>
#include <gtest/gtest.h>
#include "value_objects/pud_goal_list.hpp"

namespace {

struct PudGoalListTest : public ::testing::Test {
    expr a{expr::functor{1, {}}};
    expr b{expr::functor{2, {}}};
    expr c{expr::functor{3, {}}};

    std::vector<pud_body_goal> goals_of(const pud_goal_list& list) {
        return std::vector<pud_body_goal>(list.goals.begin(), list.goals.end());
    }
};

}

TEST_F(PudGoalListTest, AddingNumbersGoalsFromTheNextId) {
    const pud_goal_list list = pud_goal_list{.goals = {}, .next_id = 4}.adding({&a, &b});
    EXPECT_EQ(goals_of(list), (std::vector<pud_body_goal>{{.id = 4, .goal = &a}, {.id = 5, .goal = &b}}));
    EXPECT_EQ(list.next_id, 6u);
}

TEST_F(PudGoalListTest, ClosingRemovesOnlyThatGoalAndKeepsTheNextId) {
    const pud_goal_list list = pud_goal_list{.goals = {}, .next_id = 0}.adding({&a, &b, &c}).closing(1);
    EXPECT_EQ(goals_of(list), (std::vector<pud_body_goal>{{.id = 0, .goal = &a}, {.id = 2, .goal = &c}}));
    EXPECT_EQ(list.next_id, 3u);
}

TEST_F(PudGoalListTest, AChildsGoalsStayInIdOrder) {
    const pud_goal_list parent = pud_goal_list{.goals = {}, .next_id = 0}.adding({&a, &b});
    const pud_goal_list child = parent.closing(0).adding({&c});
    EXPECT_EQ(goals_of(child), (std::vector<pud_body_goal>{{.id = 1, .goal = &b}, {.id = 2, .goal = &c}}));
}

TEST_F(PudGoalListTest, ClosingLeavesTheOriginalListAlone) {
    const pud_goal_list parent = pud_goal_list{.goals = {}, .next_id = 0}.adding({&a, &b});
    parent.closing(0);
    EXPECT_EQ(parent.goals.size(), 2u);
}
