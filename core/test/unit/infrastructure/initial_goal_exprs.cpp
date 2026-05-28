// Initial goal expression list: ordered push, indexed get, and count. Indices must
// match insertion order; get throws or returns per vector semantics for bounds.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/initial_goal_exprs.hpp"

using ::testing::ElementsAre;

struct InitialGoalExprsTest : public ::testing::Test {
    initial_goal_exprs goals;
    expr e0{expr::var{0}};
    expr e1{expr::var{1}};
    expr e2{expr::var{2}};
};

TEST_F(InitialGoalExprsTest, EmptyInitially) {
    EXPECT_EQ(goals.count(), 0u);
}

TEST_F(InitialGoalExprsTest, PushIncreasesCount) {
    goals.push(&e0);
    goals.push(&e1);
    EXPECT_EQ(goals.count(), 2u);
}

TEST_F(InitialGoalExprsTest, GetOutOfRangeThrows) {
    goals.push(&e0);
    EXPECT_THROW(goals.get(1), std::out_of_range);
}

TEST_F(InitialGoalExprsTest, GetReturnsExprsInPushOrder) {
    goals.push(&e0);
    goals.push(&e1);
    goals.push(&e2);
    const std::vector<const expr*> ordered{goals.get(0), goals.get(1), goals.get(2)};
    EXPECT_THAT(ordered, ElementsAre(&e0, &e1, &e2));
}
