#include <optional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_query_holders.hpp"
#include "pud_test_node.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;

namespace {

struct PudQueryHoldersTest : public ::testing::Test {
    pud_query_holders holders;
};

}

TEST_F(PudQueryHoldersTest, AQueryNobodyHoldsHasNoHolder) {
    EXPECT_FALSE(holders.find_holder(3).has_value());
}

TEST_F(PudQueryHoldersTest, AHolderIsFoundWithTheQuerysGoal) {
    holders.hold(3, pud_test_node(8), 2);
    EXPECT_EQ(holders.find_holder(3), (pud_unfold{.leaf = pud_test_node(8), .goal = 2}));
}

TEST_F(PudQueryHoldersTest, TheLeastHolderIsFoundFirst) {
    holders.hold(3, pud_test_node(9), 2);
    holders.hold(3, pud_test_node(4), 2);
    EXPECT_EQ(holders.find_holder(3), (pud_unfold{.leaf = pud_test_node(4), .goal = 2}));
    EXPECT_THAT(holders.get_holders(3), ElementsAre(pud_test_node(4), pud_test_node(9)));
}

TEST_F(PudQueryHoldersTest, ReleasingReportsOnlyTheLastHolder) {
    holders.hold(3, pud_test_node(4), 0);
    holders.hold(3, pud_test_node(5), 0);
    EXPECT_FALSE(holders.release(3, pud_test_node(4)));
    EXPECT_TRUE(holders.release(3, pud_test_node(5)));
    EXPECT_FALSE(holders.find_holder(3).has_value());
}

TEST_F(PudQueryHoldersTest, AQueryNobodyHoldsAnyMoreHasNoHolders) {
    holders.hold(3, pud_test_node(4), 0);
    holders.release(3, pud_test_node(4));
    EXPECT_THAT(holders.get_holders(3), IsEmpty());
    EXPECT_THAT(holders.get_holders(7), IsEmpty());
}

TEST_F(PudQueryHoldersTest, QueriesAreHeldIndependently) {
    holders.hold(1, pud_test_node(4), 0);
    holders.hold(2, pud_test_node(4), 1);
    EXPECT_TRUE(holders.release(1, pud_test_node(4)));
    EXPECT_EQ(holders.find_holder(2), (pud_unfold{.leaf = pud_test_node(4), .goal = 1}));
}
