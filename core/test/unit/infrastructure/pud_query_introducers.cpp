#include <map>
#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/pud_query_introducers.hpp"
#include "pud_test_node.hpp"

namespace {

struct PudQueryIntroducersTest : public ::testing::Test {
    pud_query_introducers introducers;
};

}

TEST_F(PudQueryIntroducersTest, AnIntroducedQueryIsFoundAtItsNodeAndGoal) {
    introducers.introduce(40, pud_test_node(3), 2);
    EXPECT_EQ(introducers.find_introduction(40), (pud_query_introduction{.node = pud_test_node(3), .goal = 2}));
}

TEST_F(PudQueryIntroducersTest, AQueryNeverIntroducedIsNotFound) {
    EXPECT_FALSE(introducers.find_introduction(40).has_value());
}

TEST_F(PudQueryIntroducersTest, AWithdrawnQueryIsNoLongerFoundAndOthersStay) {
    introducers.introduce(40, pud_test_node(3), 2);
    introducers.introduce(41, pud_test_node(4), 1);
    introducers.withdraw(40);
    EXPECT_FALSE(introducers.find_introduction(40).has_value());
    EXPECT_EQ(introducers.find_introduction(41), (pud_query_introduction{.node = pud_test_node(4), .goal = 1}));
}

TEST_F(PudQueryIntroducersTest, IntroducingAQueryTwiceFails) {
    introducers.introduce(40, pud_test_node(3), 2);
    EXPECT_THROW(introducers.introduce(40, pud_test_node(4), 2), std::logic_error);
}

TEST_F(PudQueryIntroducersTest, WithdrawingAQueryNeverIntroducedFails) {
    EXPECT_THROW(introducers.withdraw(40), std::logic_error);
}

TEST_F(PudQueryIntroducersTest, AnExhaustedQueryIsListedWithItsIntroductionUntilItIsWithdrawn) {
    introducers.introduce(40, pud_test_node(3), 2);
    introducers.introduce(41, pud_test_node(4), 1);
    introducers.mark_exhausted(40);
    EXPECT_EQ(
        introducers.exhausted(),
        (std::map<pud_mhq_head_id, pud_query_introduction>{
            {40, pud_query_introduction{.node = pud_test_node(3), .goal = 2}}}));
    introducers.withdraw(40);
    EXPECT_TRUE(introducers.exhausted().empty());
}
