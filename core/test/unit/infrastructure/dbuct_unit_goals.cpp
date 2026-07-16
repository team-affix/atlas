// dbuct_unit_goals: LIFO push/pop and frame undo of queue mutations.

#include <gtest/gtest.h>
#include "infrastructure/dbuct_unit_goals.hpp"
#include "value_objects/lineage.hpp"

struct DbuctUnitGoalsTest : public ::testing::Test {
    dbuct_unit_goals goals;
    goal_lineage gl0{nullptr, 0};
    goal_lineage gl1{nullptr, 1};
};

TEST_F(DbuctUnitGoalsTest, EmptyPopIsNullopt) {
    EXPECT_EQ(goals.pop(), std::nullopt);
}

TEST_F(DbuctUnitGoalsTest, PushPopIsLifo) {
    goals.push_frame();
    goals.push(&gl0);
    goals.push(&gl1);
    EXPECT_EQ(goals.pop(), std::optional<const goal_lineage*>{&gl1});
    EXPECT_EQ(goals.pop(), std::optional<const goal_lineage*>{&gl0});
    EXPECT_EQ(goals.pop(), std::nullopt);
}

TEST_F(DbuctUnitGoalsTest, PopFrameRestoresQueue) {
    goals.push_frame();
    goals.push(&gl0);
    goals.push_frame();
    goals.push(&gl1);
    EXPECT_EQ(goals.pop(), std::optional<const goal_lineage*>{&gl1});
    goals.pop_frame();
    EXPECT_EQ(goals.pop(), std::optional<const goal_lineage*>{&gl0});
}
