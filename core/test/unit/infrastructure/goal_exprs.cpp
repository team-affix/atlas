// Goal expression store: get/set/unset per goal lineage. Missing keys must map to
// nullptr; set overwrites; unset removes without affecting other goals.

#include <gtest/gtest.h>
#include "../../../core/hpp/infrastructure/goal_exprs.hpp"
#include "../../../core/hpp/value_objects/lineage.hpp"

struct GoalExprsTest : public ::testing::Test {
    goal_exprs store;
    expr e0{expr::var{0}};
    expr e1{expr::var{1}};
    expr e2{expr::var{2}};
    goal_lineage gl0{nullptr, 0};
    goal_lineage gl1{nullptr, 1};
};

TEST_F(GoalExprsTest, GetMissingReturnsNullptr) {
    ASSERT_EQ(store.get(&gl0), nullptr);
}

TEST_F(GoalExprsTest, SetThenGetReturnsExpr) {
    store.set(&gl0, &e0);
    EXPECT_EQ(store.get(&gl0), &e0);
}

TEST_F(GoalExprsTest, SetOverwritesPreviousExpr) {
    store.set(&gl0, &e0);
    store.set(&gl0, &e1);
    EXPECT_EQ(store.get(&gl0), &e1);
}

TEST_F(GoalExprsTest, UnsetRemovesEntry) {
    store.set(&gl0, &e0);
    store.unset(&gl0);
    EXPECT_EQ(store.get(&gl0), nullptr);
}

TEST_F(GoalExprsTest, UnsetDoesNotAffectOtherGoals) {
    store.set(&gl0, &e0);
    store.set(&gl1, &e1);
    store.unset(&gl0);
    EXPECT_EQ(store.get(&gl1), &e1);
}
