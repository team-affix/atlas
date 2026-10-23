#include <optional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_query_holders.hpp"

using ::testing::ElementsAre;

namespace {

struct PudQueryHoldersTest : public ::testing::Test {
    pud_query_holders holders;
};

}

TEST_F(PudQueryHoldersTest, AQueryNobodyHoldsHasNoHolder) {
    EXPECT_FALSE(holders.find_holder(3).has_value());
}

TEST_F(PudQueryHoldersTest, AHolderIsFoundWithTheQuerysGoal) {
    holders.hold(3, 8, 2);
    EXPECT_EQ(holders.find_holder(3), (pud_unfold{.leaf = 8, .goal = 2}));
}

TEST_F(PudQueryHoldersTest, TheLeastHolderIsFoundFirst) {
    holders.hold(3, 9, 2);
    holders.hold(3, 4, 2);
    EXPECT_EQ(holders.find_holder(3), (pud_unfold{.leaf = 4, .goal = 2}));
    EXPECT_THAT(holders.get_holders(3), ElementsAre(4u, 9u));
}

TEST_F(PudQueryHoldersTest, ReleasingReportsOnlyTheLastHolder) {
    holders.hold(3, 4, 0);
    holders.hold(3, 5, 0);
    EXPECT_FALSE(holders.release(3, 4));
    EXPECT_TRUE(holders.release(3, 5));
    EXPECT_FALSE(holders.find_holder(3).has_value());
}

TEST_F(PudQueryHoldersTest, QueriesAreHeldIndependently) {
    holders.hold(1, 4, 0);
    holders.hold(2, 4, 1);
    EXPECT_TRUE(holders.release(1, 4));
    EXPECT_EQ(holders.find_holder(2), (pud_unfold{.leaf = 4, .goal = 1}));
}
