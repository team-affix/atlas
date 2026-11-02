#include <gtest/gtest.h>
#include "infrastructure/pud_root_goal_list_builder.hpp"

namespace {

using goals_t = immer::map<pud_body_goal_id, const expr*>;

struct PudRootGoalListBuilderTest : public ::testing::Test {
    pud_root_goal_list_builder opener;
    expr a{expr::functor{1, {}}};
    expr b{expr::functor{2, {}}};
    expr c{expr::functor{3, {}}};
};

}

TEST_F(PudRootGoalListBuilderTest, OpenGoalsNumbersGoalsFromZero) {
    const pud_goal_list list = opener.build({&a, &b});
    EXPECT_EQ(list.goals, goals_t{}.set(0, &a).set(1, &b));
    EXPECT_EQ(list.next_id, 2u);
}

TEST_F(PudRootGoalListBuilderTest, OpenGoalsWithAnEmptyVectorYieldsAnEmptyList) {
    const pud_goal_list list = opener.build({});
    EXPECT_EQ(list.goals, goals_t{});
    EXPECT_EQ(list.next_id, 0u);
}

TEST_F(PudRootGoalListBuilderTest, OpenGoalsKeysGoalsInBodyOrder) {
    const pud_goal_list list = opener.build({&a, &b, &c});
    EXPECT_EQ(list.goals, goals_t{}.set(0, &a).set(1, &b).set(2, &c));
    EXPECT_EQ(list.next_id, 3u);
}
