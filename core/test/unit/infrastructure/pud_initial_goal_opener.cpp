#include <gtest/gtest.h>
#include "infrastructure/pud_initial_goal_opener.hpp"

namespace {

using goals_t = immer::map<pud_body_goal_id, const expr*>;

struct PudInitialGoalOpenerTest : public ::testing::Test {
    pud_initial_goal_opener opener;
    expr a{expr::functor{1, {}}};
    expr b{expr::functor{2, {}}};
    expr c{expr::functor{3, {}}};
};

}

TEST_F(PudInitialGoalOpenerTest, OpenGoalsNumbersGoalsFromZero) {
    const pud_goal_list list = opener.open_goals({&a, &b});
    EXPECT_EQ(list.goals, goals_t{}.set(0, &a).set(1, &b));
    EXPECT_EQ(list.next_id, 2u);
}

TEST_F(PudInitialGoalOpenerTest, OpenGoalsWithAnEmptyVectorYieldsAnEmptyList) {
    const pud_goal_list list = opener.open_goals({});
    EXPECT_EQ(list.goals, goals_t{});
    EXPECT_EQ(list.next_id, 0u);
}

TEST_F(PudInitialGoalOpenerTest, OpenGoalsKeysGoalsInBodyOrder) {
    const pud_goal_list list = opener.open_goals({&a, &b, &c});
    EXPECT_EQ(list.goals, goals_t{}.set(0, &a).set(1, &b).set(2, &c));
    EXPECT_EQ(list.next_id, 3u);
}
