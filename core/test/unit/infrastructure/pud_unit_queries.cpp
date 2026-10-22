#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/pud_unit_queries.hpp"

struct PudUnitQueriesTest : public ::testing::Test {
    pud_unit_queries unit_queries;
};

TEST_F(PudUnitQueriesTest, StartsEmpty) {
    EXPECT_TRUE(unit_queries.empty());
}

TEST_F(PudUnitQueriesTest, OldestPushIsInFront) {
    unit_queries.push(3);
    unit_queries.push(7);
    EXPECT_FALSE(unit_queries.empty());
    EXPECT_EQ(unit_queries.front(), 3u);
}

TEST_F(PudUnitQueriesTest, PopUncoversTheNextOldest) {
    unit_queries.push(3);
    unit_queries.push(7);
    unit_queries.pop();
    EXPECT_EQ(unit_queries.front(), 7u);
    unit_queries.pop();
    EXPECT_TRUE(unit_queries.empty());
}

TEST_F(PudUnitQueriesTest, PushesAfterAPopQueueBehindTheRest) {
    unit_queries.push(3);
    unit_queries.push(7);
    unit_queries.pop();
    unit_queries.push(9);
    EXPECT_EQ(unit_queries.front(), 7u);
    unit_queries.pop();
    EXPECT_EQ(unit_queries.front(), 9u);
}

TEST_F(PudUnitQueriesTest, SameHeadMayBePushedTwice) {
    unit_queries.push(3);
    unit_queries.push(3);
    unit_queries.pop();
    EXPECT_EQ(unit_queries.front(), 3u);
}

TEST_F(PudUnitQueriesTest, FrontOrPopOfAnEmptyQueueIsAPreconditionViolation) {
    EXPECT_THROW(unit_queries.front(), std::logic_error);
    EXPECT_THROW(unit_queries.pop(), std::logic_error);
}
