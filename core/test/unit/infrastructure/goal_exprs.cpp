// Goal expression store: get/set/unset per goal lineage. Missing or erased keys throw
// out_of_range on get; duplicate set and invalid unset throw logic_error in debug builds.

#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/goal_exprs.hpp"
#include "value_objects/lineage.hpp"

struct GoalExprsTest : public ::testing::Test {
    goal_exprs store;
    expr e0{expr::var{0}};
    expr e1{expr::var{1}};
    goal_lineage gl0{nullptr, 0};
    goal_lineage gl1{nullptr, 1};
};

TEST_F(GoalExprsTest, GetOnUnknownGoalThrows) {
    EXPECT_THROW(store.get(&gl0), std::out_of_range);
}

TEST_F(GoalExprsTest, SetThenGetReturnsExpr) {
    store.set(&gl0, {&e0, 0});
    EXPECT_EQ(store.get(&gl0).skeleton, &e0);
}

TEST_F(GoalExprsTest, SecondSetThrows) {
    store.set(&gl0, {&e0, 0});
    EXPECT_THROW(store.set(&gl0, {&e1, 0}), std::logic_error);
}

TEST_F(GoalExprsTest, UnsetRemovesEntry) {
    store.set(&gl0, {&e0, 0});
    store.unset(&gl0);
    EXPECT_THROW(store.get(&gl0), std::out_of_range);
}

TEST_F(GoalExprsTest, UnsetOnUnknownGoalThrows) {
    EXPECT_THROW(store.unset(&gl0), std::logic_error);
}

TEST_F(GoalExprsTest, UnsetTwiceThrows) {
    store.set(&gl0, {&e0, 0});
    store.unset(&gl0);
    EXPECT_THROW(store.unset(&gl0), std::logic_error);
}

TEST_F(GoalExprsTest, UnsetDoesNotAffectOtherGoals) {
    store.set(&gl0, {&e0, 0});
    store.set(&gl1, {&e1, 0});
    store.unset(&gl0);
    EXPECT_EQ(store.get(&gl1).skeleton, &e1);
}

TEST_F(GoalExprsTest, ClearGoalExprsRemovesAllBindings) {
    store.set(&gl0, {&e0, 0});
    store.set(&gl1, {&e1, 0});
    store.clear_goal_exprs();
    EXPECT_THROW(store.get(&gl0), std::out_of_range);
    EXPECT_THROW(store.get(&gl1), std::out_of_range);
}
