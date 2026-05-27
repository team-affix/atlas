// Initial goal expression list: ordered push, indexed at, and size. Indices must
// match insertion order; at throws or returns per vector semantics for bounds.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/initial_goal_exprs.hpp"

using ::testing::ElementsAre;

struct InitialGoalExprsTest : public ::testing::Test {
    initial_goal_exprs goals;
    expr e0{expr::var{0}};
    expr e1{expr::var{1}};
    expr e2{expr::var{2}};
};

TEST_F(InitialGoalExprsTest, EmptyInitially) {
    EXPECT_EQ(goals.size(), 0u);
}

TEST_F(InitialGoalExprsTest, PushIncreasesSize) {
    goals.push(&e0);
    goals.push(&e1);
    EXPECT_EQ(goals.size(), 2u);
}

TEST_F(InitialGoalExprsTest, AtReturnsExprsInPushOrder) {
    goals.push(&e0);
    goals.push(&e1);
    goals.push(&e2);
    const std::vector<const expr*> ordered{goals.at(0), goals.at(1), goals.at(2)};
    EXPECT_THAT(ordered, ElementsAre(&e0, &e1, &e2));
}
