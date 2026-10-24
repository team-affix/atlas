#include <gtest/gtest.h>
#include "infrastructure/pud_node_goals.hpp"
#include "pud_test_node.hpp"

namespace {

using goals_t = immer::map<pud_body_goal_id, const expr*>;

struct PudNodeGoalsTest : public ::testing::Test {
    pud_node_goals goals;
    expr a{expr::functor{1, {}}};
    expr b{expr::functor{2, {}}};
};

}

TEST_F(PudNodeGoalsTest, AStoredListReadsBack) {
    const pud_goal_list list{.goals = goals_t{}.set(0, &a), .next_id = 1};
    goals.store(pud_test_node(3), list);
    EXPECT_EQ(goals.get(pud_test_node(3)), list);
}

TEST_F(PudNodeGoalsTest, FindLooksAGoalUpByItsId) {
    goals.store(
        pud_test_node(0),
        pud_goal_list{.goals = goals_t{}.set(1, &b).set(2, &a), .next_id = 3});
    EXPECT_EQ(goals.find(pud_test_node(0), 1), &b);
    EXPECT_EQ(goals.find(pud_test_node(0), 2), &a);
}
