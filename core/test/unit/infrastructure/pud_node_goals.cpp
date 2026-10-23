#include <gtest/gtest.h>
#include "infrastructure/pud_node_goals.hpp"
#include "pud_test_node.hpp"

namespace {

struct PudNodeGoalsTest : public ::testing::Test {
    pud_node_goals goals;
    expr a{expr::functor{1, {}}};
    expr b{expr::functor{2, {}}};
};

}

TEST_F(PudNodeGoalsTest, AStoredListReadsBack) {
    const pud_goal_list list = pud_goal_list{.goals = {}, .next_id = 0}.adding(&a);
    goals.store(pud_test_node(3), list);
    EXPECT_EQ(goals.get(pud_test_node(3)), list);
}

TEST_F(PudNodeGoalsTest, FindLooksAGoalUpByItsId) {
    goals.store(pud_test_node(0), pud_goal_list{.goals = {}, .next_id = 0}.adding(&a).adding(&b).closing(0).adding(&a));
    EXPECT_EQ(goals.find(pud_test_node(0), 1), &b);
    EXPECT_EQ(goals.find(pud_test_node(0), 2), &a);
}
