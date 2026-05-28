// Unit goal queue: LIFO push/pop and clear. pop returns std::nullopt when empty and
// otherwise yields the most recently pushed goal.

#include <gtest/gtest.h>
#include "../../../core/hpp/infrastructure/unit_goals.hpp"
#include "../../../core/hpp/value_objects/lineage.hpp"

struct UnitGoalsTest : public ::testing::Test {
    unit_goals queue;
    expr e0{expr::var{0}};
    expr e1{expr::var{1}};
    goal_lineage gl0{nullptr, &e0};
    goal_lineage gl1{nullptr, &e1};
};

TEST_F(UnitGoalsTest, EmptyInitially) {
    EXPECT_FALSE(queue.pop().has_value());
}

TEST_F(UnitGoalsTest, PopReturnsLastPushed) {
    queue.push(&gl0);
    queue.push(&gl1);
    auto p1 = queue.pop();
    ASSERT_TRUE(p1.has_value());
    EXPECT_EQ(*p1, &gl1);
    auto p0 = queue.pop();
    ASSERT_TRUE(p0.has_value());
    EXPECT_EQ(*p0, &gl0);
    EXPECT_FALSE(queue.pop().has_value());
}

TEST_F(UnitGoalsTest, ClearEmptiesQueue) {
    queue.push(&gl0);
    queue.clear();
    EXPECT_FALSE(queue.pop().has_value());
}
