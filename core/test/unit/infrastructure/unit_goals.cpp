// Unit goal queue: LIFO push/pop, empty check, and clear. pop must return the most
// recently pushed goal; empty queue reports true from empty().

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
    EXPECT_TRUE(queue.empty());
}

TEST_F(UnitGoalsTest, PopReturnsLastPushed) {
    queue.push(&gl0);
    queue.push(&gl1);
    EXPECT_EQ(queue.pop(), &gl1);
    EXPECT_EQ(queue.pop(), &gl0);
    EXPECT_TRUE(queue.empty());
}

TEST_F(UnitGoalsTest, ClearEmptiesQueue) {
    queue.push(&gl0);
    queue.clear();
    EXPECT_TRUE(queue.empty());
}
